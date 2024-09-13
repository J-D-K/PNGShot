#include <malloc.h>
#include <png.h>
#include <stdint.h>
#include <string.h>
#include <switch.h>

// This is used to jump to the cleanup label
#define GoToCleanupIf(x)                                                                                               \
    if (x)                                                                                                             \
    goto cleanup

// Just in case this stuff changes.
#define SCREENSHOT_WIDTH 1280
#define SCREENSHOT_HEIGHT 720
#define SCREENSHOT_BIT_DEPTH 8
// The timeout for screen capture
#define SCREENSHOT_CAPTURE_TIMEOUT 1e+8

// This copies RGBADataIn to RGBDataOut, just skipping the alpha byte.
#ifdef EXPERIMENTAL
static inline void RGBAToRGB(restrict png_const_bytep RGBADataIn, png_bytepp RGBDataOut)
{
    size_t i = 0;
    // Loop through array of rows
    for (int currentRow = 0; currentRow < SCREENSHOT_HEIGHT; currentRow++)
    {
        png_bytep row = RGBDataOut[currentRow];
        for (size_t j = 0; j < SCREENSHOT_WIDTH * 3; j += 3, i += 4)
        {
            memcpy(&row[j], &RGBADataIn[i], 3);
        }
    }
}
#else
static inline void RGBAToRGB(restrict png_const_bytep RGBADataIn, png_bytep RGBDataOut)
{
    for (size_t i = 0, j = 0; i < SCREENSHOT_WIDTH * 4; i += 4, j += 3)
    {
        // We're only copying the 3 RGB bytes.
        memcpy(&RGBDataOut[j], &RGBADataIn[i], 3);
    }
}
#endif

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
    png_bytepp RGBData = NULL;
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
    // If any of this is null, cleanup
    GoToCleanupIf(PngWritingStruct == NULL || PngInfoStruct == NULL || RGBAData == NULL);

    // This needs to be an array for png_write_image
    RGBData = malloc(sizeof(png_bytep *) * SCREENSHOT_HEIGHT);
    GoToCleanupIf(RGBData == NULL);

    // Init each row
    for (int i = 0; i < SCREENSHOT_HEIGHT; i++)
    {
        RGBData[i] = malloc(SCREENSHOT_WIDTH * 3);
        GoToCleanupIf(RGBAData == NULL);
    }

    // Try to open file for writing.
    PngFile = fopen(PngPath, "wb");
    GoToCleanupIf(PngFile == NULL);

    // Bytes read.
    uint64_t BytesRead = 0;
    // Read stream to RGBA Buffer
    CapsscError = capsscReadRawScreenShotReadStream(&BytesRead, RGBAData, CaptureSize, 0);
    if (R_FAILED(CapsscError))
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
    // Convert RGBA to RGB
    RGBAToRGB(RGBAData, RGBData);
    // Write image
    png_write_image(PngWritingStruct, RGBData);

cleanup:
    png_write_end(PngWritingStruct, PngInfoStruct);
    png_free_data(PngWritingStruct, PngInfoStruct, PNG_FREE_ALL, -1);
    png_destroy_write_struct(&PngWritingStruct, &PngInfoStruct);
    free(RGBAData);
    // Free array of rows.
    for (int i = 0; i < SCREENSHOT_HEIGHT; i++)
    {
        free(RGBData[i]);
    }
    free(RGBData);
    fclose(PngFile);
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
