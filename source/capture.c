#include <malloc.h>
#include <png.h>
#include <stdint.h>
#include <string.h>
#include <switch.h>

// Just in case this stuff changes.
#define SCREENSHOT_WIDTH 1280
#define SCREENSHOT_HEIGHT 720
#define SCREENSHOT_BIT_DEPTH 8
// The timeout for screen capture
#define SCREENSHOT_CAPTURE_TIMEOUT 1e+8

// This copies RGBADataIn to RGBDataOut, just skipping the alpha byte.
static inline void RGBAToRGB(restrict png_const_bytep RGBADataIn, png_bytep RGBDataOut)
{
    for (size_t i = 0, j = 0; i < SCREENSHOT_WIDTH * 4; i += 4, j += 3)
    {
        // We're only copying the 3 RGB bytes.
        memcpy(&RGBDataOut[j], &RGBADataIn[i], 3);
    }
}

// Which one gets used depends on flag sent to makefile
#ifdef EXPERIMENTAL
// To do: Clean this version up to look nicer.
void CaptureScreenShot(const char *PngPath)
{
    // These are actually always know, and the same, but for some reason we need them to write to?
    uint64_t CaptureSize = 0;
    uint64_t CaptureWidth = 0;
    uint64_t CaptureHeight = 0;
    // LibPNG Structs and buffers for reading/writing RGB(A) data from system.
    png_structp PngWritingStruct = NULL;
    png_infop PngInfoStruct = NULL;
    png_bytep RGBAData = NULL;
    png_bytep RGBData = NULL;
    // File pointer we're going to write to.
    FILE *PngFile = NULL;

    // Before anything else, try to get the stream.
    Result CapsscError = capsscOpenRawScreenShotReadStream(&CaptureSize,
                                                           &CaptureWidth,
                                                           &CaptureHeight,
                                                           ViLayerStack_Screenshot,
                                                           SCREENSHOT_CAPTURE_TIMEOUT);
    if (R_FAILED(CapsscError))
    {
        // Just bail before anything else takes place.
        return;
    }

    // Try to allocate everything before continuing.
    PngWritingStruct = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    PngInfoStruct = png_create_info_struct(PngWritingStruct);
    RGBAData = malloc(CaptureSize);
    RGBData = malloc(SCREENSHOT_WIDTH * 3);
    PngFile = fopen(PngPath, "wb");

    // If anything failed, goto cleanup.
    if (PngWritingStruct == NULL || PngInfoStruct == NULL || RGBAData == NULL || RGBData == NULL || PngFile == NULL)
    {
        goto cleanup;
    }

    // Bytes read.
    uint64_t BytesRead = 0;
    // Read stream to RGBA Buffer
    CapsscError = capsscReadRawScreenShotReadStream(&BytesRead, RGBAData, CaptureSize, 0);
    if(R_FAILED(CapsscError))
    {
        goto cleanup;
    }
    // Close stream.
    capsscCloseRawScreenShotReadStream();

    // Get everything ready.
    png_init_io(PngWritingStruct, PngFile);
    png_set_IHDR(PngWritingStruct,
                 PngInfoStruct,
                 SCREENSHOT_WIDTH,
                 SCREENSHOT_HEIGHT,
                 SCREENSHOT_BIT_DEPTH,
                 PNG_COLOR_TYPE_RGB,
                 PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT,
                 PNG_FILTER_TYPE_DEFAULT);
    png_write_info(PngWritingStruct, PngInfoStruct);
    // Loop through row by row.
    for (int i = 0; i < SCREENSHOT_HEIGHT; i++)
    {
        // Offset in RGBAData buffer to use
        size_t RGBAOffset = i * (SCREENSHOT_WIDTH * 4);
        // Remove alpha since there's no point in saving it here.
        RGBAToRGB(&RGBAData[RGBAOffset], RGBData);
        // Write the row to file
        png_write_row(PngWritingStruct, RGBData);
    }

cleanup:
    png_write_end(PngWritingStruct, PngInfoStruct);
    png_free_data(PngWritingStruct, PngInfoStruct, PNG_FREE_ALL, -1);
    png_destroy_write_struct(&PngWritingStruct, &PngInfoStruct);
    free(RGBAData);
    free(RGBData);
    fclose(PngFile);
    capsscCloseRawScreenShotReadStream();
}
#else
void CaptureScreenShot(const char *PngPath)
{
    // These are actually always know, and the same, but for some reason we need them to write to?
    uint64_t CaptureSize = 0;
    uint64_t CaptureWidth = 0;
    uint64_t CaptureHeight = 0;
    // LibPNG Structs and buffers for reading/writing RGB(A) data from system.
    png_structp PngWritingStruct = NULL;
    png_infop PngInfoStruct = NULL;
    png_bytep RGBAData = NULL;
    png_bytep RGBData = NULL;
    // File pointer we're going to write to.
    FILE *PngFile = NULL;

    // Before anything else, try to get the stream.
    Result CapsscError = capsscOpenRawScreenShotReadStream(&CaptureSize,
                                                           &CaptureWidth,
                                                           &CaptureHeight,
                                                           ViLayerStack_Screenshot,
                                                           SCREENSHOT_CAPTURE_TIMEOUT);
    if (R_FAILED(CapsscError))
    {
        // Just bail before anything else takes place.
        return;
    }

    // Try to allocate everything before continuing.
    PngWritingStruct = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    PngInfoStruct = png_create_info_struct(PngWritingStruct);
    RGBAData = malloc((SCREENSHOT_WIDTH * 4) * 2);
    RGBData = malloc((SCREENSHOT_WIDTH * 3) * 2);
    PngFile = fopen(PngPath, "wb");

    // If anything failed, goto cleanup.
    if (PngWritingStruct == NULL || PngInfoStruct == NULL || RGBAData == NULL || RGBData == NULL || PngFile == NULL)
    {
        goto cleanup;
    }

    // Get everything ready.
    png_init_io(PngWritingStruct, PngFile);
    png_set_IHDR(PngWritingStruct,
                 PngInfoStruct,
                 SCREENSHOT_WIDTH,
                 SCREENSHOT_HEIGHT,
                 SCREENSHOT_BIT_DEPTH,
                 PNG_COLOR_TYPE_RGB,
                 PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT,
                 PNG_FILTER_TYPE_DEFAULT);
    png_write_info(PngWritingStruct, PngInfoStruct);
    // Loop through two rows at a time.
    for (int i = 0; i < SCREENSHOT_HEIGHT; i++)
    {
        // Number of bytes read from stream.
        uint64_t BytesRead = 0;
        // Try to read from the stream
        CapsscError =
            capsscReadRawScreenShotReadStream(&BytesRead, RGBAData, SCREENSHOT_WIDTH * 4, i * (SCREENSHOT_WIDTH * 4));
        if (R_FAILED(CapsscError))
        {
            goto cleanup;
        }
        // Remove alpha since there's no point in saving it here.
        RGBAToRGB(RGBAData, RGBData);
        // Write the row to file
        png_write_row(PngWritingStruct, RGBData);
    }

cleanup:
    png_write_end(PngWritingStruct, PngInfoStruct);
    png_free_data(PngWritingStruct, PngInfoStruct, PNG_FREE_ALL, -1);
    png_destroy_write_struct(&PngWritingStruct, &PngInfoStruct);
    free(RGBAData);
    free(RGBData);
    fclose(PngFile);
    capsscCloseRawScreenShotReadStream();
}
#endif
