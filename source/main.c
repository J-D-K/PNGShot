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

// Libnx fake heap stuff for sysmodules
#define INNER_HEAP_SIZE 0x80000
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

// This is so we can properly call this function from libnx after time is initialized. The devkitpro example doesn't even build?
extern void __libnx_init_time(void);

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

// Just makes sure the target output directory exists.
inline void createOutputDirectory(void)
{
    // Get current time.
    time_t currentTime;
    time(&currentTime);
    struct tm *localTime = localtime(&currentTime); // This is bad and I *should* rename this variable if I feel like it.

    // Full path we're using.
    char outputPath[FS_MAX_PATH];
    snprintf(outputPath, FS_MAX_PATH, "sdmc:/switch/PNGShot/%04d-%02d-%02d", localTime->tm_year + 1900, localTime->tm_mon + 1, localTime->tm_mday);

    // Make sure it exists. Mode doesn't matter for switch?
    mkdir(outputPath, 0x777);
}

inline void generateScreenshotPathAndName(char *pathOut, int maxLength)
{
    // Get local time for name.
    time_t currentTime;
    time(&currentTime);
    struct tm *localTime = localtime(&currentTime);
    // Sprintf the file name and pray.
    snprintf(pathOut, maxLength, "sdmc:/switch/PNGShot/%04d-%02d-%02d/%02d-%02d-%02d.png", localTime->tm_year + 1900, localTime->tm_mon + 1, localTime->tm_mday, localTime->tm_hour, localTime->tm_min, localTime->tm_sec);
}

// This is to strip the alpha byte from the raw data. Screenshots are smaller this way and there's no reason to save it.
// Source is always RGBA comimg from capssc, destination should be 1280 * 3 bytes.
inline void stripAlphaByteFromRow(png_bytep sourceData, png_bytep destinationData)
{
    for(int i = 0; i < SCREENSHOT_WIDTH * 4; i += 4)
    {
        // Copy RGB
        for(int j = 0; j < 3; j++)
        {
            *destinationData++ = *sourceData++;
        }
        // Skip alpha byte
        sourceData++;
    }
}

// This function actually captures the screenshot.
void captureScreenShot(void)
{
    // These are actually always known, but for some reason capssc needs to write to them?
    uint64_t captureSize;
    uint64_t captureWidth;
    uint64_t captureHeight;

    Result capsscError = capsscOpenRawScreenShotReadStream(&captureSize, &captureWidth, &captureHeight, ViLayerStack_Screenshot, 100000000);
    if (R_FAILED(capsscError))
    {
        // Just bail if it failed.
        return;
    }

    // Start libpng for writing. Might wanna retouch this later for errors...
    png_structp pngWriteStruct = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop pngInfoStruct = png_create_info_struct(pngWriteStruct);
    if (pngWriteStruct != NULL && pngInfoStruct != NULL)
    {
        // To-do: Clean this up and error check better before an actual release...
        // Just to be sure it exists.
        createOutputDirectory();

        // Get screenshot name.
        char screenshotPath[FS_MAX_PATH];
        generateScreenshotPathAndName(screenshotPath, FS_MAX_PATH);

        // File
        FILE *screenshotFile = fopen(screenshotPath, "wb");

        // Allocate read and write rows. First is for raw RGBA from system, second is just RGB so the screenshots are smaller and don't waste space.
        png_bytep sourceData = malloc(SCREENSHOT_WIDTH * 4);
        png_bytep destinationData = malloc(SCREENSHOT_WIDTH * 3);

        // Start png output
        png_init_io(pngWriteStruct, screenshotFile);
        png_set_IHDR(pngWriteStruct, pngInfoStruct, SCREENSHOT_WIDTH, SCREENSHOT_HEIGHT, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
        png_write_info(pngWriteStruct, pngInfoStruct);
        // Loop row by row through capture stream.
        for(int i = 0; i < 720; i++)
        {
            // Number of bytes read.
            uint64_t bytesRead = 0;
            // Read capture into sourceRow.
            capsscReadRawScreenShotReadStream(&bytesRead, sourceData, SCREENSHOT_WIDTH * 4, i * (SCREENSHOT_WIDTH * 4));
            // Strip the alpha byte and output into destinationRow
            stripAlphaByteFromRow(sourceData, destinationData);
            // Write to the png
            png_write_row(pngWriteStruct, (png_const_bytep)destinationData);
        }
        // To do: Use the devil's keyword to use this.
        cleanup:
            png_write_end(pngWriteStruct, NULL);
            png_free_data(pngWriteStruct, pngInfoStruct, PNG_FREE_ALL, -1);
            png_destroy_write_struct(&pngWriteStruct, NULL);
            free(sourceData);
            free(destinationData);
            fclose(screenshotFile);
    }
    // Don't forget to close this if it's open.
    capsscCloseRawScreenShotReadStream();
}

int main(void)
{
    // Get event handle for capture button
    Event captureButtton;
    Result hidsysError = hidsysAcquireCaptureButtonEventHandle(&captureButtton, false);
    // Should probably error check that, but meh.
    eventClear(&captureButtton);

    // Make sure base directory exists.
    mkdir("sdmc:/switch/PNGShot", 0777);

    // Loop forever and ever and ever and ever.
    while(true)
    {
        if(R_SUCCEEDED(eventWait(&captureButtton, UINT64_MAX)))
        {
            // Clear the event.
            eventClear(&captureButtton);
            // Capture the screenshot layer.
            captureScreenShot();
        }
    }
}
