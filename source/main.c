#include <malloc.h>
#include <png.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
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
// The timeout for screen capture
#define SCREENSHOT_CAPTURE_TIMEOUT 1e+8

// Libnx fake heap stuff for sysmodules
#define INNER_HEAP_SIZE 0x60000

// This is so we can properly call this function from libnx after the time service is Initialized.
extern void __libnx_init_time(void);

// Initializes heap
void __libnx_initheap(void)
{
    // "Heap" memory we're using.
    static char InnerHeap[INNER_HEAP_SIZE];

    // Pointers to libnx/newlib heap pointers.
    extern char *fake_heap_start, *fake_heap_end;

    // Make it point to the place ya know
    fake_heap_start = InnerHeap;
    fake_heap_end = InnerHeap + INNER_HEAP_SIZE;
}

// Init services and stuff needed.
void __appInit(void)
{
    Result Initialize = smInitialize();
    if (R_FAILED(Initialize))
    {
        diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_InitFail_SM));
    }

    Initialize = setsysInitialize();
    if (R_SUCCEEDED(Initialize))
    {
        SetSysFirmwareVersion firmwareVersion;
        Result setsysError = setsysGetFirmwareVersion(&firmwareVersion);
        if (R_SUCCEEDED(setsysError))
        {
            hosversionSet(MAKEHOSVERSION(firmwareVersion.major, firmwareVersion.minor, firmwareVersion.micro));
        }
        setsysExit();
    }

    Initialize = hidsysInitialize();
    if (R_FAILED(Initialize))
    {
        diagAbortWithResult(MAKERESULT(Module_Libnx, -4));
    }

    Initialize = fsInitialize();
    if (R_FAILED(Initialize))
    {
        diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_InitFail_FS));
    }

    Initialize = capsscInitialize();
    if (R_FAILED(Initialize))
    {
        // Just make this one up?
        diagAbortWithResult(MAKERESULT(Module_Libnx, -1));
    }

    Initialize = timeInitialize();
    if (R_SUCCEEDED(Initialize))
    {
        __libnx_init_time();
        timeExit();
    }
    else
    {
        diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_InitFail_Time));
    }

    Initialize = fsdevMountSdmc();
    if (R_FAILED(Initialize))
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
    capsscExit();
    fsExit();
    hidsysExit();
}

// This writes the name of the screenshot to ScreeShotNameBuffer.
inline void GenerateScreenShotPathAndName(char *ScreenShotNameBuffer, int MaximumLength)
{
    // Grab local time.
    time_t Timer;
    time(&Timer);
    struct tm *LocalTime = localtime(&Timer);
    strftime(ScreenShotNameBuffer, MaximumLength, "sdmc:/switch/PNGShot/%Y%m%d%H%M%S.png", LocalTime);
}

// This copies RGBADataIn to RGBDataOut, just skipping the alpha byte.
inline void RGBAToRGB(restrict png_const_bytep RGBADataIn, png_bytep RGBDataOut)
{
    for (size_t i = 0, j = 0; i < SCREENSHOT_WIDTH * 4; i += 4, j += 3)
    {
        // We're only copying the 3 RGB bytes.
        memcpy(&RGBDataOut[j], &RGBADataIn[i], 3);
    }
}

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

int main(void)
{
    // Get event handle for capture button
    Event CaptureButton;
    // Calling this with auto clear fails.
    Result HidSysError = hidsysAcquireCaptureButtonEventHandle(&CaptureButton, false);
    if (R_FAILED(HidSysError))
    {
        // Just return and pretend PNGShot was never here to begin with...
        return -1;
    }
    // Clear event
    eventClear(&CaptureButton);

    // This just assumes the sdmc:/switch folder exists already.
    mkdir("sdmc:/switch/PNGShot", 0777);

    // Loop forever, waiting for capture button event.
    while (true)
    {
        if (R_SUCCEEDED(eventWait(&CaptureButton, UINT64_MAX)))
        {
            // Clear the event first.
            eventClear(&CaptureButton);
            // Get new screenshot name.
            char PngPath[FS_MAX_PATH];
            GenerateScreenShotPathAndName(PngPath, FS_MAX_PATH);
            // Capture
            CaptureScreenShot(PngPath);
        }
    }
    return 0;
}
