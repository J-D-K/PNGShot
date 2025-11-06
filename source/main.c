#include "FSFILE.h"
#include "init.h"
#include "jpeg.h"
#include "png_capture.h"

#include <stdbool.h>
#include <stdio.h>
#include <switch.h>

// Macro to fail with. Not the right error code but whatever.
#define ABORT_ON_FAILURE(x)                                                                                                    \
    if (R_FAILED(x)) diagAbortWithResult(MAKERESULT(Module_Libnx, x))

// Libnx sysmodule stuff.
uint32_t __nx_applet_type     = AppletType_None;
uint32_t __nx_fs_num_sessions = 1;

#define INNER_HEAP_SIZE 0x60000

// Initializes heap
void __libnx_initheap(void)
{
    // "Heap" memory we're using.
    static char innerHeap[INNER_HEAP_SIZE];

    // Pointers to libnx/newlib heap pointers.
    extern char *fake_heap_start, *fake_heap_end;

    // Make it point to the place ya know
    fake_heap_start = innerHeap;
    fake_heap_end   = innerHeap + INNER_HEAP_SIZE;
}

// I just think this looks a bit nicer?
static inline void init_libnx_firm_version(void)
{
    const bool setSysError = R_FAILED(setsysInitialize());
    if (setSysError) { return; }

    SetSysFirmwareVersion firmVersion;
    const bool firmError = R_FAILED(setsysGetFirmwareVersion(&firmVersion));
    if (firmError)
    {
        setsysExit();
        return;
    }

    hosversionSet(MAKEHOSVERSION(firmVersion.major, firmVersion.minor, firmVersion.micro));
    setsysExit();
}

void __appInit(void)
{
    ABORT_ON_FAILURE(smInitialize());
    init_libnx_firm_version();
    ABORT_ON_FAILURE(hidsysInitialize());
    ABORT_ON_FAILURE(fsInitialize());
    ABORT_ON_FAILURE(capsscInitialize());
    // Exit sm, it's not needed anymore.
    smExit();
}

void __appExit(void)
{
    capsscExit();
    fsdevUnmountAll();
    fsExit();
    hidsysExit();
}

int main(void)
{
    // Thresholds for capturing.
    static const uint64_t UPPER_THRESHOLD = 500000000;
    static const uint64_t LOWER_THRESHOLD = 50000000;

    // Get the capture handle before continuing.
    Event captureButton;
    ABORT_ON_FAILURE(hidsysAcquireCaptureButtonEventHandle(&captureButton, false));
    ABORT_ON_FAILURE(eventClear(&captureButton));

    // Check for the flag to allow jpegs to be captured too.
    jpeg_check_for_flag();

    // Open album directory and make sure folder exists.
    FsFileSystem albumDir;
    if (!init_open_album_directory(&albumDir)) { return -1; }
    else if (!init_create_pngshot_directory(&albumDir)) { return -2; }

    bool captureHeld    = false; // Tracks whether the button was held.
    uint64_t beginTicks = 0;     // The beginning tick number when the button was first pressed.
    while (true)
    {
        // Wait for the capture button to be pressed.
        const bool capturePressed = R_SUCCEEDED(eventWait(&captureButton, UINT64_MAX));
        const bool captureCleared = R_SUCCEEDED(eventClear(&captureButton));
        // I guess technically it's possible for the timeout to expire.
        if (!capturePressed && !captureCleared) { continue; }

        // Calculate this stuff.
        const uint64_t systemTicks = armGetSystemTick();
        const uint64_t elapsedNano = armTicksToNs(systemTicks - beginTicks);

        // Conditions for capture.
        // There's a ghost press when the system first starts.
        const bool beginHold  = elapsedNano >= UPPER_THRESHOLD;
        const bool validPress = captureHeld && elapsedNano >= LOWER_THRESHOLD && elapsedNano < UPPER_THRESHOLD;
        if (beginHold)
        {
            beginTicks  = systemTicks;
            captureHeld = true;
        }
        else if (validPress)
        {
            png_capture(&albumDir);
            captureHeld = false;
        }
        else { captureHeld = false; }
    }

    return 0;
}
