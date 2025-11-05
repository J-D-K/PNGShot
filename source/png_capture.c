#include "FSFILE.h"
#include "fsdir.h"
#include "jpeg.h"

#include <ctype.h> // Include for tolower
#include <malloc.h>
#include <png.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <switch.h>
#include <time.h>

// These are used in a couple of different places.
static const int SCREENSHOT_WIDTH  = 1280;
static const int SCREENSHOT_HEIGHT = 720;

// This is a temporary name used to write the PNG. It's moved and renamed afterwards.
static const char *TEMPORARY_NAME = "/PNGs/temp.png";

// Defined at bottom.
// These are needed to make libpng work with the raw FS commands.
static void png_write_function(png_structp writingStruct, png_bytep pngData, png_size_t length);
static void png_flush_function(png_structp writingStruct);

/// @brief Attempts to open the capssc capture stream. Returns false on failure.
static bool capssc_open_stream();

/// @brief Reads a row from the stream into the buffer passed.
/// @param buffer Row buffer to read into.
/// @param rowIndex Current row height-wise to read.
/// @return True on success. False on failure.
static bool capssc_read_row(void *buffer, int rowIndex);

/// @brief Initializes the structs for PNG writing. Returns false on failure.
/// @param writeStruct Pointer to writing struct pointer.
/// @param infoStruct Pointer to info struct pointer.
static bool png_init_structs(png_structpp writeStruct, png_infopp infoStruct);

/// @brief Cleans up png write operations.
/// @param writeStruct Write struct to free.
/// @param infoStruct Infostruct to free.
static void png_cleanup(png_structpp writeStruct, png_infopp infoStruct);

/// @brief Inits the I/O functions for writing the png and writes info to the png.
/// @param writeStruct PNG write struct we're using.
/// @param file FSFILE we're writing to.
static void png_init_io_write_info(png_structp writeStruct, png_infop infoStruct, FSFILE *file);

/// @brief Shifts all of the bytes over in the row passed and "deletes" the alpha value from the screenshot since it's not
/// needed.
static inline void rgba_strip_alpha(restrict png_bytep row);

/// @brief Creates the end target directory for the screenshot to go to.
/// @param timestamp Timestamp to use to generate the path.
static bool create_target_directory(FsFileSystem *filesystem, uint64_t timestamp);

/// @brief Renames (or moves) the screenshot to its final destination.
/// @param filesystem Filesystem the screenshot was created on.
/// @param timestamp Timestamp to use to name the screenshot.
static void move_rename_screenshot(FsFileSystem *filesystem, uint64_t timestamp);

// Same as above, but safer and less memory hungry for a Switch sysmodule
void png_capture(FsFileSystem *filesystem)
{
    png_structp writeStruct = NULL;
    png_infop infoStruct    = NULL;
    FSFILE *pngFile         = NULL;
    uint8_t rowBuffer[SCREENSHOT_WIDTH * sizeof(uint32_t)];

    // Open stream and init libPNG structs.
    if (!capssc_open_stream()) { return; }
    else if (!png_init_structs(&writeStruct, &infoStruct)) { return; }

    // Attempt to open temporary output file.
    pngFile = FSFILE_Open(filesystem, TEMPORARY_NAME);
    if (!pngFile) { goto cleanup; }

    // Initialize libpng to use our write functions and write the initial info.
    png_init_io_write_info(writeStruct, infoStruct, pngFile);

    // Loop through the rows of the capture.
    for (size_t i = 0; i < SCREENSHOT_HEIGHT; i++)
    {
        // Read the next row.
        const bool rowRead = capssc_read_row(rowBuffer, i);
        if (!rowRead) { goto cleanup; }

        // Shift everything and delete the alpha values.
        rgba_strip_alpha(rowBuffer);

        // Write the RGBA row with libpng stripping the alpha channel
        png_write_row(writeStruct, rowBuffer);
    }

cleanup:
    // This will finalize writing and destroy the structs.
    png_cleanup(&writeStruct, &infoStruct);
    FSFILE_Close(pngFile);
    capsscCloseRawScreenShotReadStream();

    FsTimeStampRaw timestamp;
    if (R_FAILED(fsFsGetFileTimeStampRaw(filesystem, TEMPORARY_NAME, &timestamp))) { return; }

    // Ensure the final directory exists.
    if (!create_target_directory(filesystem, timestamp.created)) { return; }

    // Move the screenshot.
    move_rename_screenshot(filesystem, timestamp.created);

    // Delete the jpeg if needed.
    if (jpeg_needs_deletion()) { jpeg_delete_capture(filesystem, timestamp.created); }
}

static void png_write_function(png_structp writingStruct, png_bytep pngData, png_size_t length)
{
    FSFILE *fsfile = (FSFILE *)png_get_io_ptr(writingStruct);
    FSFILE_Write(fsfile, pngData, length);
}

static void png_flush_function(png_structp writingStruct)
{
    FSFILE *fsfile = (FSFILE *)png_get_io_ptr(writingStruct);
    FSFILE_Flush(fsfile);
}

static bool capssc_open_stream()
{
    // The timeout for screen capture
    static const int64_t SCREENSHOT_CAPTURE_TIMEOUT = 1e+8;

    uint64_t size;
    uint64_t width;
    uint64_t height;
    const bool opened = R_SUCCEEDED(
        capsscOpenRawScreenShotReadStream(&size, &width, &height, ViLayerStack_Screenshot, SCREENSHOT_CAPTURE_TIMEOUT));
    return opened;
}

static bool capssc_read_row(void *buffer, int rowOffset)
{
    // This is always the same.
    static const size_t ROW_SIZE = SCREENSHOT_WIDTH * 4; // Each row is RGBA.

    // Read the row at the offset.
    uint64_t bytesRead;
    const bool rowRead = R_SUCCEEDED(capsscReadRawScreenShotReadStream(&bytesRead, buffer, ROW_SIZE, rowOffset * ROW_SIZE));
    return rowRead && bytesRead == ROW_SIZE;
}

static bool png_init_structs(png_structpp writeStruct, png_infopp infoStruct)
{
    *writeStruct = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!*writeStruct) { return false; }

    *infoStruct = png_create_info_struct(*writeStruct);
    if (!*infoStruct)
    {
        png_destroy_write_struct(writeStruct, NULL);
        return false;
    }

    png_set_compression_level(*writeStruct, 9);
    return true;
}

static void png_cleanup(png_structpp writeStruct, png_infopp infoStruct)
{
    if (!*writeStruct && !*infoStruct) { return; }

    png_write_end(*writeStruct, *infoStruct);
    png_free_data(*writeStruct, *infoStruct, PNG_FREE_ALL, -1);
    png_destroy_write_struct(writeStruct, infoStruct);
}

static void png_init_io_write_info(png_structp writeStruct, png_infop infoStruct, FSFILE *file)
{
    // Just in case this stuff changes.
    static const int SCREENSHOT_BIT_DEPTH = 8;

    // Make libpng use our functions instead of stdio.
    png_set_write_fn(writeStruct, file, png_write_function, png_flush_function);

    // Set IHDR
    png_set_IHDR(writeStruct,
                 infoStruct,
                 SCREENSHOT_WIDTH,
                 SCREENSHOT_HEIGHT,
                 SCREENSHOT_BIT_DEPTH,
                 PNG_COLOR_TYPE_RGB,
                 PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT,
                 PNG_FILTER_TYPE_DEFAULT);

    // Write the info.
    png_write_info(writeStruct, infoStruct);
}

static inline void rgba_strip_alpha(restrict png_bytep row)
{
    for (int i = 0, j = 0; i < SCREENSHOT_WIDTH * 4; i += 4, j += 3)
    {
        row[j]     = row[i];
        row[j + 1] = row[i + 1];
        row[j + 2] = row[i + 2];
    }
}

static bool create_target_directory(FsFileSystem *filesystem, uint64_t timestamp)
{
    // This just makes stuff easier to read and work with.
    struct tm localTime = *localtime((const time_t *)&timestamp);

    // Generate the end path.
    char pathBuffer[FS_MAX_PATH] = {0};
    snprintf(pathBuffer,
             FS_MAX_PATH,
             "/PNGs/%04d/%02d/%02d/",
             localTime.tm_year + 1900,
             localTime.tm_mon + 1,
             localTime.tm_mday);

    // Check if it exists first. If it does, don't continue.
    const bool exists = directory_exists(filesystem, pathBuffer);
    if (exists) { return true; }

    // Try to create it.
    return create_directory_recursively(filesystem, pathBuffer);
}

static void move_rename_screenshot(FsFileSystem *filesystem, uint64_t timestamp)
{
    // Convert this to something easier to work with.
    struct tm localTime = *localtime((const time_t *)&timestamp);

    // Construct the final path.
    char finalPath[FS_MAX_PATH] = {0};
    snprintf(finalPath,
             FS_MAX_PATH,
             "/PNGs/%04d/%02d/%02d/%04d-%02d-%02d_%02d-%02d-%02d.png",
             localTime.tm_year + 1900,
             localTime.tm_mon + 1,
             localTime.tm_mday,
             localTime.tm_year + 1900,
             localTime.tm_mon + 1,
             localTime.tm_mday,
             localTime.tm_hour,
             localTime.tm_min,
             localTime.tm_sec);

    // Move/rename. There's no real point in error checking this.
    fsFsRenameFile(filesystem, TEMPORARY_NAME, finalPath);
}
