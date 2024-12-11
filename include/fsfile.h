#pragma once
#include <switch.h>

// This is a bare minimum struct/wrapper to make file writing using libnx file functions easier.
typedef struct
{
    FsFile fileHandle;
    int64_t fileOffset;
} FSFILE;

// This stuff is named like this to avoid conflicts with libnx.
// Returns if file could be opened for reading. This is just to ping to see if the no_jpeg file exists.
bool FSFILEExists(FsFileSystem *filesystem, const char *filePath);
// Opens file for writing. Returns NULL on failure.
FSFILE *FSFILEOpen(FsFileSystem *filesystem, const char *filePath);
// Writes buffer to file. Returns 0 on failure.
size_t FSFILEWrite(FSFILE *file, void *buffer, size_t bufferSize);
// Flushes file
void FSFILEFlush(FSFILE *file);
// Closes buffer.
void FSFILEClose(FSFILE *file);
