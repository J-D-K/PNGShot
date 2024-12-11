#include "fsfile.h"
#include <ctype.h> // Include for tolower
#include <malloc.h>
#include <png.h>
#include <stdint.h>
#include <string.h>
#include <switch.h>

// This is used to jump to the cleanup label.
#define CLEANUP_AND_ABORT_IF(x)                                                                                                                \
    if (x)                                                                                                                                     \
    goto cleanup

// This returns from the funtion.
#define RETURN_ON_FAILURE(x)                                                                                                                   \
    if (R_FAILED(x))                                                                                                                           \
    return

#define CLEANUP_AND_ABORT_ON_FAILURE(x)                                                                                                        \
    if (R_FAILED(x))                                                                                                                           \
    goto cleanup

// Just in case this stuff changes.
#define SCREENSHOT_WIDTH 1280
#define SCREENSHOT_HEIGHT 720
#define SCREENSHOT_BIT_DEPTH 8
// The timeout for screen capture
#define SCREENSHOT_CAPTURE_TIMEOUT 1e+8

// This is whether or not to nuke the jpeg after the PNG is taken.
bool g_NoJpeg = true;

// These functions are needed so we can use libpng with libnx's file functions.
void pngWriteFunction(png_structp writingStruct, png_bytep pngData, png_size_t length)
{
    FSFILEWrite((FSFILE *)png_get_io_ptr(writingStruct), pngData, length);
}

void pngFlushFunction(png_structp writingStruct)
{
    FSFILEFlush((FSFILE *)png_get_io_ptr(writingStruct));
}

// Lib png functions.
void initLibPNGStructs(png_structpp writingStruct, png_infopp infoStruct)
{
    *writingStruct = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    *infoStruct = png_create_info_struct(*writingStruct);
}
//
void pngInitIOAndWriteInfo(png_structp writingStruct, png_infop infoStruct, FSFILE *pngFile)
{
    png_set_write_fn(writingStruct, pngFile, pngWriteFunction, pngFlushFunction);
    png_set_IHDR(writingStruct,
                 infoStruct,
                 SCREENSHOT_WIDTH,
                 SCREENSHOT_HEIGHT,
                 SCREENSHOT_BIT_DEPTH,
                 PNG_COLOR_TYPE_RGB,
                 PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT,
                 PNG_FILTER_TYPE_DEFAULT);
    png_write_info(writingStruct, infoStruct);
}

// Function to delete the .jpg file closest to the specified reference timestamp in the Album directory
void deleteClosestToCurrentTimeJpg(FsFileSystem *albumDirectory, u64 referenceTimestamp)
{
    FsDir rootDir;
    FsDirectoryEntry rootEntry;
    char closestFilePath[FS_MAX_PATH] = {0};
    u64 smallestTimeDelta = (u64)(-1); // Initialize to max possible value

    // Open the Album directory
    if (R_FAILED(fsFsOpenDirectory(albumDirectory, "/", FsDirOpenMode_ReadDirs, &rootDir)))
    {
        return;
    }

    s64 entriesRead = 0;
    while (R_SUCCEEDED(fsDirRead(&rootDir, &entriesRead, 1, &rootEntry)) && entriesRead > 0)
    {
        if (rootEntry.type != FsDirEntryType_Dir)
            continue;

        char yearFolder[FS_MAX_PATH];
        snprintf(yearFolder, FS_MAX_PATH, "/%s", rootEntry.name);
        FsDir yearDir;
        if (R_FAILED(fsFsOpenDirectory(albumDirectory, yearFolder, FsDirOpenMode_ReadDirs, &yearDir)))
            continue;

        FsDirectoryEntry monthEntry;
        s64 monthEntriesRead = 0;
        while (R_SUCCEEDED(fsDirRead(&yearDir, &monthEntriesRead, 1, &monthEntry)) && monthEntriesRead > 0)
        {
            if (monthEntry.type != FsDirEntryType_Dir)
                continue;

            char monthFolder[FS_MAX_PATH];
            snprintf(monthFolder, FS_MAX_PATH, "%s/%s", yearFolder, monthEntry.name);
            FsDir monthDir;
            if (R_FAILED(fsFsOpenDirectory(albumDirectory, monthFolder, FsDirOpenMode_ReadDirs, &monthDir)))
                continue;

            FsDirectoryEntry dayEntry;
            s64 dayEntriesRead = 0;
            while (R_SUCCEEDED(fsDirRead(&monthDir, &dayEntriesRead, 1, &dayEntry)) && dayEntriesRead > 0)
            {
                if (dayEntry.type != FsDirEntryType_Dir)
                    continue;

                char dayFolder[FS_MAX_PATH];
                snprintf(dayFolder, FS_MAX_PATH, "%s/%s", monthFolder, dayEntry.name);
                FsDir dayDir;
                if (R_FAILED(fsFsOpenDirectory(albumDirectory, dayFolder, FsDirOpenMode_ReadFiles, &dayDir)))
                    continue;

                FsDirectoryEntry fileEntry;
                s64 fileEntriesRead = 0;
                while (R_SUCCEEDED(fsDirRead(&dayDir, &fileEntriesRead, 1, &fileEntry)) && fileEntriesRead > 0)
                {
                    if (fileEntry.type != FsDirEntryType_File)
                        continue;

                    // Check for ".jpg" extension
                    size_t len = strlen(fileEntry.name);
                    if (len < 4 || strcmp(&fileEntry.name[len - 4], ".jpg") != 0)
                        continue;

                    // Parse timestamp from filename
                    u64 fileTimestamp = 0;
                    for (int i = 0; i < 14 && isdigit((unsigned char)fileEntry.name[i]); ++i)
                    {
                        fileTimestamp = fileTimestamp * 10 + (fileEntry.name[i] - '0');
                    }

                    // Calculate the time difference from the reference timestamp
                    u64 timeDelta =
                        (fileTimestamp > referenceTimestamp) ? (fileTimestamp - referenceTimestamp) : (referenceTimestamp - fileTimestamp);

                    // Update if this .jpg file is the closest to the reference timestamp
                    if (timeDelta < smallestTimeDelta)
                    {
                        smallestTimeDelta = timeDelta;
                        snprintf(closestFilePath, FS_MAX_PATH, "%s/%s", dayFolder, fileEntry.name);
                    }
                }
                fsDirClose(&dayDir);
            }
            fsDirClose(&monthDir);
        }
        fsDirClose(&yearDir);
    }
    fsDirClose(&rootDir);

    // Delete the closest file if found
    if (smallestTimeDelta != (u64)(-1))
    {
        fsFsDeleteFile(albumDirectory, closestFilePath);
    }
}

// Function to create a timestamped filename and return the timestamp
u64 generateTimestampedFilename(char *pathOut, int pathMaxLength, FsFileSystem *fs, const char *tempPath)
{
    FsTimeStampRaw timestamp;
    u64 fileTimestamp = 0; // Initialize to hold the timestamp

    if (R_SUCCEEDED(fsFsGetFileTimeStampRaw(fs, tempPath, &timestamp)))
    {
        time_t ts = timestamp.created;
        struct tm t;

        if (gmtime_r(&ts, &t))
        {
            // Generate timestamp in yyyymmddhhmmss format
            fileTimestamp = (t.tm_year + 1900) * 10000000000ULL + (t.tm_mon + 1) * 100000000 + t.tm_mday * 1000000 + t.tm_hour * 10000 +
                            t.tm_min * 100 + t.tm_sec;

            // Use the timestamp to format the file path
            snprintf(pathOut,
                     pathMaxLength,
                     "/PNGs/%d-%02d-%02d_%02d-%02d-%02d.png",
                     t.tm_year + 1900,
                     t.tm_mon + 1,
                     t.tm_mday,
                     t.tm_hour,
                     t.tm_min,
                     t.tm_sec);
        }
    }
    return fileTimestamp; // Return the timestamp
}


// Same as above, but safer and less memory hungry for a Switch sysmodule
void captureScreenshot(FsFileSystem *filesystem, const char *tempFilePath)
{
    uint64_t captureSize = 0;
    uint64_t captureWidth = 0;
    uint64_t captureHeight = 0;
    png_structp pngWritingStruct = NULL;
    png_infop pngInfoStruct = NULL;
    png_bytep sourceRow = NULL;
    FSFILE *pngFile = NULL;

    RETURN_ON_FAILURE(
        capsscOpenRawScreenShotReadStream(&captureSize, &captureWidth, &captureHeight, ViLayerStack_Screenshot, SCREENSHOT_CAPTURE_TIMEOUT));

    initLibPNGStructs(&pngWritingStruct, &pngInfoStruct);
    CLEANUP_AND_ABORT_IF(pngWritingStruct == NULL || pngInfoStruct == NULL);

    sourceRow = malloc(SCREENSHOT_WIDTH * 4); // Single RGBA row buffer
    CLEANUP_AND_ABORT_IF(sourceRow == NULL);

    pngFile = FSFILEOpen(filesystem, tempFilePath); // Save to temporary path
    CLEANUP_AND_ABORT_IF(pngFile == NULL);

    pngInitIOAndWriteInfo(pngWritingStruct, pngInfoStruct, pngFile);

    // Set libpng to ignore the alpha channel in the RGBA data
    png_set_filler(pngWritingStruct, 0, PNG_FILLER_AFTER);

    for (size_t i = 0; i < SCREENSHOT_HEIGHT; i++)
    {
        uint64_t bytesRead = 0;
        CLEANUP_AND_ABORT_ON_FAILURE(
            capsscReadRawScreenShotReadStream(&bytesRead, sourceRow, SCREENSHOT_WIDTH * 4, i * (SCREENSHOT_WIDTH * 4)));

        // Write the RGBA row with libpng stripping the alpha channel
        png_write_row(pngWritingStruct, sourceRow);
    }

cleanup:
    png_write_end(pngWritingStruct, pngInfoStruct);
    png_free_data(pngWritingStruct, pngInfoStruct, PNG_FREE_ALL, -1);
    png_destroy_write_struct(&pngWritingStruct, &pngInfoStruct);
    free(sourceRow);
    FSFILEClose(pngFile);
    capsscCloseRawScreenShotReadStream();

    // Rename file based on timestamp
    char finalPath[FS_MAX_PATH];
    u64 timestamp = generateTimestampedFilename(finalPath, FS_MAX_PATH, filesystem, tempFilePath);
    fsFsRenameFile(filesystem, tempFilePath, finalPath);

    if (g_NoJpeg)
    {
        deleteClosestToCurrentTimeJpg(filesystem, timestamp);
    }
}
