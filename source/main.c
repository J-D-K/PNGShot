#include <stdio.h>
#include <switch.h>
#include <sys/stat.h>
#include <time.h>
#include "capture.h"

// Libnx sysmodule stuff.
uint32_t __nx_applet_type = AppletType_None;
uint32_t __nx_fs_num_sessions = 1;

// Experimental build buffers the entire screenshot stream and needs more memory to work with.
#ifdef EXPERIMENTAL
#define INNER_HEAP_SIZE 0x3E4000
#else
#define INNER_HEAP_SIZE 0x60000
#endif

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
