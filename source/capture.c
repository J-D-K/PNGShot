#include "FSFILE.h"

#include <ctype.h> // Include for tolower
#include <malloc.h>
#include <png.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <switch.h>
#include <time.h>

// This is used to jump to the cleanup label in capture
#define CLEANUP_AND_ABORT_IF(x)                                                                                                \
    if (x) goto cleanup

// This is whether or not to nuke the jpeg after the PNG is taken.
bool g_noJpeg = true;

// Just in case this stuff changes.
static const int SCREENSHOT_WIDTH     = 1280;
static const int SCREENSHOT_HEIGHT    = 720;
static const int SCREENSHOT_BIT_DEPTH = 8;

// Temp file name for screen shots.
static const char *TEMPORARY_FILE_NAME = "/PNGs/temp.png";

// The timeout for screen capture
static const int64_t SCREENSHOT_CAPTURE_TIMEOUT = 1e+8;

// These functions are needed so we can use libpng with libnx's file functions.
void png_write_function(png_structp writingStruct, png_bytep pngData, png_size_t length)
{
    FSFILE *fsfile = (FSFILE *)png_get_io_ptr(writingStruct);
    FSFILE_Write(fsfile, pngData, length);
}

void png_flush_function(png_structp writingStruct)
{
    FSFILE *fsfile = (FSFILE *)png_get_io_ptr(writingStruct);
    FSFILE_Flush(fsfile);
}

// For some reason, doing this results in smaller png files than using png_set_filler, so I'm sticking with it.
static inline void rgbaStripAlpha(restrict png_bytep rgbaData)
{
    // This should be able to use the same buffer and do both.
    for (int i = 0, j = 0; i < SCREENSHOT_WIDTH * 4; i += 4, j += 3) { memcpy(&rgbaData[j], &rgbaData[i], 3); }
}

// Same as above, but safer and less memory hungry for a Switch sysmodule
void captureScreenshot(FsFileSystem *filesystem)
{
    png_structp pngWritingStruct = NULL;
    png_infop pngInfoStruct      = NULL;
    FSFILE *pngFile              = NULL;

    // Stack allocated to avoid more heap allocations.
    unsigned char sourceRow[SCREENSHOT_WIDTH * sizeof(uint32_t)];

    {
        // These are useless and a waste of stack space.
        uint64_t captureSize   = 0;
        uint64_t captureWidth  = 0;
        uint64_t captureHeight = 0;
        Result capsError       = capsscOpenRawScreenShotReadStream(&captureSize,
                                                             &captureWidth,
                                                             &captureHeight,
                                                             ViLayerStack_Screenshot,
                                                             SCREENSHOT_CAPTURE_TIMEOUT);
        if (R_FAILED(capsError)) { return; }
    }

    pngWritingStruct = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    pngInfoStruct    = png_create_info_struct(pngWritingStruct);
    CLEANUP_AND_ABORT_IF(!pngWritingStruct || !pngInfoStruct);

    pngFile = FSFILE_Open(filesystem, TEMPORARY_FILE_NAME); // Save to temporary path
    CLEANUP_AND_ABORT_IF(!pngFile);

    png_set_write_fn(pngWritingStruct, pngFile, png_write_function, png_flush_function);
    png_set_IHDR(pngWritingStruct,
                 pngInfoStruct,
                 SCREENSHOT_WIDTH,
                 SCREENSHOT_HEIGHT,
                 SCREENSHOT_BIT_DEPTH,
                 PNG_COLOR_TYPE_RGB,
                 PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT,
                 PNG_FILTER_TYPE_DEFAULT);
    png_write_info(pngWritingStruct, pngInfoStruct);

    for (size_t i = 0; i < SCREENSHOT_HEIGHT; i++)
    {
        uint64_t bytesRead = 0;
        if (R_FAILED(
                capsscReadRawScreenShotReadStream(&bytesRead, sourceRow, SCREENSHOT_WIDTH * 4, i * (SCREENSHOT_WIDTH * 4))))
        {
            goto cleanup;
        }
        // Strip alpha byte;
        rgbaStripAlpha(sourceRow);
        // Write the RGBA row with libpng stripping the alpha channel
        png_write_row(pngWritingStruct, sourceRow);
    }

cleanup:
    png_write_end(pngWritingStruct, pngInfoStruct);
    png_free_data(pngWritingStruct, pngInfoStruct, PNG_FREE_ALL, -1);
    png_destroy_write_struct(&pngWritingStruct, &pngInfoStruct);
    FSFILE_Close(pngFile);
    capsscCloseRawScreenShotReadStream();

    FsTimeStampRaw timestamp;
    if (R_FAILED(fsFsGetFileTimeStampRaw(filesystem, TEMPORARY_FILE_NAME, &timestamp))) { return; }

    // Final name we're using.
    char finalPath[FS_MAX_PATH];
    struct tm *localTime = localtime((const time_t *)&timestamp.created);
    snprintf(finalPath,
             FS_MAX_PATH,
             "/PNGs/%04d-%02d-%02d_%02d-%02d-%02d.png",
             localTime->tm_year + 1900,
             localTime->tm_mon + 1,
             localTime->tm_mday,
             localTime->tm_hour,
             localTime->tm_min,
             localTime->tm_sec);

    // Rename it and hope it works.
    fsFsRenameFile(filesystem, TEMPORARY_FILE_NAME, finalPath);

    if (g_noJpeg) { deleteJPEGCapture(filesystem, timestamp.created); }
}
