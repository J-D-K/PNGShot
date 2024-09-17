#include "capture.h"
#include <stdio.h>
#include <switch.h>
#include <sys/stat.h>
#include <time.h>

// Macro to fail with. Not the right error code but whatever.
#define ABORT_ON_FAILURE(x)                                                                                                                    \
    if (R_FAILED(x))                                                                                                                           \
    diagAbortWithResult(MAKERESULT(Module_Libnx, x))

// Libnx sysmodule stuff.
uint32_t __nx_applet_type = AppletType_None;
uint32_t __nx_fs_num_sessions = 1;

// Experimental build buffers the entire screenshot stream and needs more memory to work with.
#ifdef EXPERIMENTAL
#define INNER_HEAP_SIZE 0x687000
#else
#define INNER_HEAP_SIZE 0x60000
#endif

// This is so we can properly call this function from libnx after the time service is Initialized.
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

// I just think this looks a bit nicer?
static inline void initLibnxFirmwareVersion(void)
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

static inline void initLibnxTime(void)
{
    ABORT_ON_FAILURE(timeInitialize());
    __libnx_init_time();
    timeExit();
}

void __appInit(void)
{
    ABORT_ON_FAILURE(smInitialize());
    initLibnxFirmwareVersion();
    initLibnxTime();
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

// This tries to open the album directory first on SD, if that fails, NAND.
static inline Result openAlbumDirectory(FsFileSystem *filesystem)
{
    // Try to open SD album first. If it fails, fall back to NAND.
    Result fsError = fsOpenImageDirectoryFileSystem(filesystem, FsImageDirectoryId_Sd);
    if(R_FAILED(fsError))
    {
        fsError = fsOpenImageDirectoryFileSystem(filesystem, FsImageDirectoryId_Nand);
    }
    return fsError;
}

// This checks for and creates the PNGShot directory if it doesn't exist.
static inline Result createPNGShotDirectory(FsFileSystem *filesystem)
{
    // Check if it exists first.
    FsDir directoryHandle;
    Result fsError = fsFsOpenDirectory(filesystem, "/PNGs", FsDirOpenMode_ReadDirs | FsDirOpenMode_ReadFiles, &directoryHandle);
    if(R_FAILED(fsError))
    {
        // If it failed, assume it doesn't exist and try to create it.
        fsError = fsFsCreateDirectory(filesystem, "/PNGs");
    }
    // Close it if it's open.
    fsDirClose(&directoryHandle);
    return fsError;
}

// This writes the name of the screenshot to ScreeShotNameBuffer.
static inline void generateScreenShotName(char *pathOut, int pathMaxLength)
{
    // Grab local time.
    time_t timer;
    time(&timer);
    strftime(pathOut, pathMaxLength, "/PNGs/%Y%m%d%H%M%S.png", localtime(&timer));
}

int main(void)
{
    // Get event handle for capture button
    Event captureButtonEvent;
    // Calling this with auto clear fails.
    ABORT_ON_FAILURE(hidsysAcquireCaptureButtonEventHandle(&captureButtonEvent, false));
    eventClear(&captureButtonEvent);

    // Open album directory and make sure folder exists.
    FsFileSystem albumDirectory;
    ABORT_ON_FAILURE(openAlbumDirectory(&albumDirectory));
    ABORT_ON_FAILURE(createPNGShotDirectory(&albumDirectory));

    // Loop forever, waiting for capture button event.
    while (true)
    {
        if (R_SUCCEEDED(eventWait(&captureButtonEvent, UINT64_MAX)))
        {
            // Clear
            eventClear(&captureButtonEvent);
            // Get path
            char screenshotPath[FS_MAX_PATH];
            generateScreenShotName(screenshotPath, FS_MAX_PATH);
            // Capture
            captureScreenshot(&albumDirectory, screenshotPath);
        }
    }
    return 0;
}
