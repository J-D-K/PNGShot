#include <malloc.h>
#include <png.h>
#include <stdio.h>
#include <stdint.h>
#include <switch.h>
#include <sys/stat.h>
#include <time.h>

// Libnx sysmodule stuff.
uint32_t __nx_applet_type = AppletType_None;
uint32_t __nx_fs_num_sessions = 1;

// Just in case this stuff changes.
#define SCREENSHOT_WIDTH 1280
#define SCREENSHOT_HEIGHT 720
#define SCREENSHOT_BIT_DEPTH 8

// Libnx fake heap stuff for sysmodules
#define INNER_HEAP_SIZE 0x60000

// This is so we can properly call this function from libnx after the time service is initialized.
extern void __libnx_init_time(void);

// Initializes heap
void __libnx_initheap(void)
{
    // "Heap" memory we're using.
    static char innerHeap[INNER_HEAP_SIZE];

    // Pointers to libnx/newlib heap pointers.
    extern char *fake_heap_start, *fake_heap_end;

    // Make it point to the place ya know
    fake_heap_start = innerHeap;
    fake_heap_end = innerHeap + INNER_HEAP_SIZE;
}

// Init services and stuff needed.
void __appInit(void)
{
    Result initialize = smInitialize();
    if (R_FAILED(initialize))
    {
        diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_InitFail_SM));
    }

    initialize = setsysInitialize();
    if (R_SUCCEEDED(initialize))
    {
        SetSysFirmwareVersion firmwareVersion;
        Result setsysError = setsysGetFirmwareVersion(&firmwareVersion);
        if (R_SUCCEEDED(setsysError))
        {
            hosversionSet(MAKEHOSVERSION(firmwareVersion.major, firmwareVersion.minor, firmwareVersion.micro));
        }
        setsysExit();
    }

    initialize = hidsysInitialize();
    if (R_FAILED(initialize))
    {
        diagAbortWithResult(MAKERESULT(Module_Libnx, -4));
    }

    initialize = fsInitialize();
    if (R_FAILED(initialize))
    {
        diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_InitFail_FS));
    }

    initialize = capsscInitialize();
    if (R_FAILED(initialize))
    {
        // Just make this one up?
        diagAbortWithResult(MAKERESULT(Module_Libnx, -1));
    }

    initialize = timeInitialize();
    if (R_FAILED(initialize))
    {
        diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_InitFail_Time));
    }

    __libnx_init_time();

    initialize = fsdevMountSdmc();
    if (R_FAILED(initialize))
    {
        diagAbortWithResult(MAKERESULT(Module_Libnx, -2));
    }

    // Exit sm, it's not needed anymore.
    smExit();
}

// Exit services.
void __appExit(void)
{
    // Just reverse order cause it makes me happy.
    fsdevUnmountAll();
    timeExit();
    capsscExit();
    fsExit();
    hidsysExit();
}

inline void generateScreenshotPathAndName(char *pathOut, int maxLength)
{
    // Get local time for name.
    time_t currentTime;
    time(&currentTime);
    struct tm *localTime = localtime(&currentTime);
    // Sprintf the file name and pray.
    snprintf(pathOut, maxLength, "sdmc:/switch/PNGShot/%04d%02d%02d%02d%02d%02d.png", localTime->tm_year + 1900, localTime->tm_mon + 1, localTime->tm_mday, localTime->tm_hour, localTime->tm_min, localTime->tm_sec);
}

// This is to strip the alpha byte from the raw data. Screenshots are smaller this way and there's no reason to save it.
// Source is always RGBA comimg from capssc, destination should be 1280 * 3 bytes.
inline void RGBAToRGB(png_const_bytep sourceData, png_bytep destinationData)
{
    for (int i = 0; i < SCREENSHOT_WIDTH * 4; i += 4)
    {
        // Copy RGB
        for (int j = 0; j < 3; j++)
        {
            *destinationData++ = *sourceData++;
        }
        // Skip alpha byte
        sourceData++;
    }
}

// This function actually captures the screenshot.
void captureScreenshot(const char *filePath)
{
    // These are actually always known, but for some reason capssc needs to write to them?
    uint64_t captureSize = 0;
    uint64_t captureWidth = 0;
    uint64_t captureHeight = 0;
    // Libpng stuff
    png_structp pngWriteStruct = NULL;
    png_infop pngInfoStruct = NULL;
    png_bytep sourceData = NULL;
    png_bytep destinationData = NULL;
    // File we're writing to.
    FILE *pngFile = NULL;

    Result capsscError = capsscOpenRawScreenShotReadStream(&captureSize, &captureWidth, &captureHeight, ViLayerStack_Screenshot, 1e+8);
    if (R_FAILED(capsscError))
    {
        // Just bail if it failed.
        return;
    }

    // Init libpng structs.
    pngWriteStruct = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    pngInfoStruct = png_create_info_struct(pngWriteStruct);
    // Allocate buffers
    sourceData = malloc(SCREENSHOT_WIDTH * 4);      // This holds RGBA
    destinationData = malloc(SCREENSHOT_WIDTH * 3); // This holds RGB
    // Try to open file
    pngFile = fopen(filePath, "wb");
    // If anything is NULL, cleanup and bail.
    if (pngWriteStruct == NULL || pngInfoStruct == NULL || sourceData == NULL || destinationData == NULL || pngFile == NULL)
    {
        goto cleanup;
    }

    // Init libpng stuff.
    png_init_io(pngWriteStruct, pngFile);
    png_set_IHDR(pngWriteStruct, pngInfoStruct, SCREENSHOT_WIDTH, SCREENSHOT_HEIGHT, SCREENSHOT_BIT_DEPTH, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_write_info(pngWriteStruct, pngInfoStruct);
    // Loop row by row, convert and write.
    for (int i = 0; i < SCREENSHOT_HEIGHT; i++)
    {
        // Number of bytes read from file.
        uint64_t bytesRead = 0;
        // Read next row from stream
        capsscError = capsscReadRawScreenShotReadStream(&bytesRead, sourceData, SCREENSHOT_WIDTH * 4, i * (SCREENSHOT_WIDTH * 4));
        // Remove alpha value before writing.
        RGBAToRGB(sourceData, destinationData);
        // Write to png.
        png_write_row(pngWriteStruct, destinationData);
    }
    // Normally, no but here it's kind of needed.
    cleanup:
    png_write_end(pngWriteStruct, pngInfoStruct);
    png_free_data(pngWriteStruct, pngInfoStruct, PNG_FREE_ALL, -1);
    png_destroy_write_struct(&pngWriteStruct, &pngInfoStruct);
    free(sourceData);
    free(destinationData);
    fclose(pngFile);
    // Don't forget to close this if it's open. Should just fail if it failed earlier.
    capsscCloseRawScreenShotReadStream();
}

int main(void)
{
    // Get event handle for capture button
    Event captureButton;
    Result hidsysError = hidsysAcquireCaptureButtonEventHandle(&captureButton, false);
    if (R_FAILED(hidsysError))
    {
        // Just silently return and pretend PNGShot was never here to begin with...
        return -1;
    }
    // Should probably error check that, but meh.
    eventClear(&captureButton);

    // Make sure base directory exists.
    mkdir("sdmc:/switch/PNGShot", 0777);

    // Loop forever and ever and ever and ever.
    while (true)
    {
        if (R_SUCCEEDED(eventWait(&captureButton, UINT64_MAX)))
        {
            // Clear the event.
            eventClear(&captureButton);
            // Generate png name.
            char screenshotName[FS_MAX_PATH];
            generateScreenshotPathAndName(screenshotName, FS_MAX_PATH);
            // Capture the screenshot layer.
            captureScreenshot(screenshotName);
        }
    }
    return 0;
}
