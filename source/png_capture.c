#include "FSFILE.h"
#include "capssc.h"
#include "capture.h"
#include "config.h"
#include "fsdir.h"
#include "jpeg.h"
#include "rgba.h"
#include "screenshot.h"

#include <ctype.h> // Include for tolower
#include <malloc.h>
#include <png.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <switch.h>
#include <time.h>

// This is a temporary name used to write the PNG. It's moved and renamed afterwards.
static const char *TEMPORARY_NAME = "/PNGs/temp.png";

// Defined at bottom.

// These are needed to make libpng work with the raw FS commands.
static void png_write_function(png_structp writingStruct, png_bytep pngData, png_size_t length);
static void png_flush_function(png_structp writingStruct);

/// @brief Initializes the structs for PNG writing. Returns false on failure.
/// @param writeStruct Pointer to writing struct pointer.
/// @param infoStruct Pointer to info struct pointer.
static inline bool png_init_structs(png_structpp writeStruct, png_infopp infoStruct);

/// @brief Cleans up png write operations.
/// @param writeStruct Write struct to free.
/// @param infoStruct Infostruct to free.
static inline void png_cleanup(png_structpp writeStruct, png_infopp infoStruct);

/// @brief Inits the I/O functions for writing the png and writes info to the png.
/// @param writeStruct PNG write struct we're using.
/// @param file FSFILE we're writing to.
static inline void png_init_io_write_info(png_structp writeStruct, png_infop infoStruct, FSFILE *file);

/// @brief Creates the end target directory for the screenshot to go to.
/// @param timestamp Timestamp to use to generate the path.
static inline bool create_target_directory(FsFileSystem *filesystem, uint64_t timestamp);

/// @brief Renames (or moves) the screenshot to its final destination.
/// @param filesystem Filesystem the screenshot was created on.
/// @param timestamp Timestamp to use to name the screenshot.
static inline void move_rename_screenshot(FsFileSystem *filesystem, uint64_t timestamp);

// Same as above, but safer and less memory hungry for a Switch sysmodule
void capture(FsFileSystem *albumDir)
{
    // File size for a full, uncompressed capture.
    static const int64_t FILE_SIZE = 0x2A4470;

    png_structp writeStruct = NULL;
    png_infop infoStruct    = NULL;
    FSFILE *pngFile         = NULL;
    uint8_t rowBuffer[SCREENSHOT_WIDTH * sizeof(uint32_t)];

    // Open stream and init libPNG structs.
    if (!capssc_open_stream()) { return; }
    else if (!png_init_structs(&writeStruct, &infoStruct)) { return; }

    // Attempt to open temporary output file.
    pngFile = FSFILE_OpenWrite(albumDir, TEMPORARY_NAME, FILE_SIZE);
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
    FSFILE_Finalize(pngFile);
    capsscCloseRawScreenShotReadStream();

    FsTimeStampRaw timestamp;
    if (R_FAILED(fsFsGetFileTimeStampRaw(albumDir, TEMPORARY_NAME, &timestamp))) { return; }

    // Ensure the final directory exists.
    if (!create_target_directory(albumDir, timestamp.created)) { return; }

    // Move the screenshot.
    move_rename_screenshot(albumDir, timestamp.created);

    // Delete the jpeg if needed.
    const ConfigStruct *config = config_get();
    if (!config->allowJpegs) { jpeg_delete_capture(albumDir, timestamp.created); }
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

static inline bool png_init_structs(png_structpp writeStruct, png_infopp infoStruct)
{
    *writeStruct = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!*writeStruct) { return false; }

    *infoStruct = png_create_info_struct(*writeStruct);
    if (!*infoStruct)
    {
        png_destroy_write_struct(writeStruct, NULL);
        return false;
    }

    const ConfigStruct *config = config_get();
    png_set_compression_level(*writeStruct, config->pngLevel);

    return true;
}

static inline void png_cleanup(png_structpp writeStruct, png_infopp infoStruct)
{
    if (!*writeStruct && !*infoStruct) { return; }

    png_write_end(*writeStruct, *infoStruct);
    png_free_data(*writeStruct, *infoStruct, PNG_FREE_ALL, -1);
    png_destroy_write_struct(writeStruct, infoStruct);
}

static inline void png_init_io_write_info(png_structp writeStruct, png_infop infoStruct, FSFILE *file)
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

static inline bool create_target_directory(FsFileSystem *filesystem, uint64_t timestamp)
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

static inline void move_rename_screenshot(FsFileSystem *filesystem, uint64_t timestamp)
{
    // Convert this to something easier to work with.
    struct tm localTime = *localtime((const time_t *)&timestamp);

    // Construct the final path.
    char finalPath[FS_MAX_PATH] = {0};
    snprintf(finalPath,
             FS_MAX_PATH,
             "/PNGs/%04d/%02d/%02d/%04d%02d%02d_%02d%02d%02d.png",
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
