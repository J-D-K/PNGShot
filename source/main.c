#include "FSFILE.h"
#include "init.h"
#include "jpeg.h"
#include "png_capture.h"

#include <stdio.h>
#include <switch.h>
#include <sys/stat.h>

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
    if (R_SUCCEEDED(setsysInitialize()))
    {
        SetSysFirmwareVersion firmwareVersion;
        if (R_SUCCEEDED(setsysGetFirmwareVersion(&firmwareVersion)))
        {
            hosversionSet(MAKEHOSVERSION(firmwareVersion.major, firmwareVersion.minor, firmwareVersion.micro));
        }
        setsysExit();
    }
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
    // Get event handle for capture button
    Event captureButtonEvent;
    // Calling this with auto-clear fails.
    ABORT_ON_FAILURE(hidsysAcquireCaptureButtonEventHandle(&captureButtonEvent, false));
    eventClear(&captureButtonEvent);

    // Detects if sdmc:/config/PNGShot/allow_jpegs exists and sets the global bool
    jpeg_check_for_flag();

    // Open album directory and make sure folder exists.
    FsFileSystem albumDir;
    if (!init_open_album_directory(&albumDir)) { return -1; }
    else if (!init_create_pngshot_directory(&albumDir)) { return -2; }

    bool held      = false; // Track if the button is held
    u64 start_tick = 0;     // Time when the button press started/

    const u64 upperThreshold = 500000000;
    const u64 lowerThreshold = 50000000;

    // Loop forever, waiting for capture button event.
    while (true)
    {
        // Check for button press event
        if (R_SUCCEEDED(eventWait(&captureButtonEvent, UINT64_MAX))) // await indefinitely
        {
            eventClear(&captureButtonEvent);

            // If the button was held for more than 500 ms, reset
            u64 elapsed_ns = armTicksToNs(armGetSystemTick() - start_tick);

            if (elapsed_ns >= upperThreshold || !held) // More than 500 ms
            {
                // If button was not already held, start holding
                held       = true;
                start_tick = armGetSystemTick();
            }
            else
            {
                // If button was already held and now released
                if (elapsed_ns >= lowerThreshold && elapsed_ns < upperThreshold) // Between 50 ms and 500 ms
                {
                    // Valid quick press detected, proceed to capture screenshot to temp path
                    png_capture(&albumDir);
                }
                // Reset the state
                held       = false;
                start_tick = 0;
            }
        }
    }

    return 0;
}
