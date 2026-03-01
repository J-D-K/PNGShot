#include "FSFILE.h"

#include <malloc.h>

/// @brief This is just so we have structure.
enum FileModes : uint8_t
{
    Reading,
    Writing
};

// clang-format off
/// @brief Actual FSFILE struct.
struct FSFILE
{
    /// @brief The actual handle.
    FsFile handle;

    /// @brief The mode in which the file is opened.
    uint8_t mode;

    /// @brief Size of the file.
    int64_t size;

    /// @brief Current offset.
    int64_t offset;
};
// clang-format on

bool FSFILE_Exists(FsFileSystem *filesystem, const char *path)
{
    // Just try to open the file for reading. If it fails, return false.
    FsFile handle;
    const bool openError = R_FAILED(fsFsOpenFile(filesystem, path, FsOpenMode_Read, &handle));
    if (openError) { return false; }

    // Close the handle and return true because it exists.
    fsFileClose(&handle);
    return true;
}

bool FSFILE_Delete(FsFileSystem *filesystem, const char *path)
{
    // Don't bother if it doesn't exist.
    const bool exists = FSFILE_Exists(filesystem, path);
    if (!exists) { return false; }

    return R_SUCCEEDED(fsFsDeleteFile(filesystem, path));
}

FSFILE *FSFILE_OpenRead(FsFileSystem *filesystem, const char *path)
{
    // Allocate.
    FSFILE *file = malloc(sizeof(FSFILE));
    if (!file) { goto abort; }

    // Attempt opening before continuing at all.
    const bool opened = R_SUCCEEDED(fsFsOpenFile(filesystem, path, FsOpenMode_Read, &file->handle));
    if (!opened) { goto abort; }

    // Get the size.
    const bool getSize = R_SUCCEEDED(fsFileGetSize(&file->handle, &file->size));
    if (!getSize) { goto abort; }

    // Set offset and mode.
    file->offset = 0;
    file->mode   = Reading;

    // Success ending achieved.
    return file;

abort:
    if (file)
    {
        // Not sure there's really a way to check this and screw adding a bool for it.
        fsFileClose(&file->handle);
        free(file);
    }

    return NULL;
}

FSFILE *FSFILE_OpenWrite(FsFileSystem *filesystem, const char *path, int64_t size)
{
    // Preliminary stuff before anything really important.
    const bool exists  = FSFILE_Exists(filesystem, path);
    const bool deleted = exists && FSFILE_Delete(filesystem, path);
    if (exists && !deleted) { goto abort; }

    // Try to create it.
    const bool created = R_SUCCEEDED(fsFsCreateFile(filesystem, path, size, 0));
    if (!created) { goto abort; }

    // Allocate.
    FSFILE *file = malloc(sizeof(FSFILE));
    if (!file) { goto abort; }

    // Open.
    const bool opened = R_SUCCEEDED(fsFsOpenFile(filesystem, path, FsOpenMode_Write, &file->handle));
    if (!opened) { goto abort; }

    // Offset, size, and mode.
    file->offset = 0;
    file->size   = size;
    file->mode   = Writing;

    return file;

abort:
    if (file) { free(file); }

    return NULL;
}

ssize_t FSFILE_Read(FSFILE *file, void *buffer, size_t size)
{
    if (!file || !buffer || file->mode != Reading) { return -1; }

    // Read.
    uint64_t bytesRead = 0;
    const bool read    = R_SUCCEEDED(fsFileRead(&file->handle, file->offset, buffer, size, FsReadOption_None, &bytesRead));
    if (!read) { return -1; }

    // Update offset.
    file->offset += bytesRead;

    return (ssize_t)bytesRead;
}

ssize_t FSFILE_Write(FSFILE *file, const void *buffer, size_t size)
{
    // Do not continue if we're not passed valid data.
    if (!file || !buffer || file->mode != Writing) { return -1; }

    // Whether or not the file needs to be resized.
    const int64_t endSize  = file->offset + size;
    const bool needsResize = endSize > file->size;
    const bool resized     = needsResize && R_SUCCEEDED(fsFileSetSize(&file->handle, endSize));
    if (needsResize && !resized) { return -1; }

    // Attempt to write the data.
    const bool dataWritten = R_SUCCEEDED(fsFileWrite(&file->handle, file->offset, buffer, size, FsWriteOption_None));
    if (!dataWritten) { return -1; }

    // Update the offset and size if needed.
    file->offset += size;
    file->size = file->offset > file->size ? file->offset : file->size;

    return size;
}

ssize_t FSFILE_GetSize(FSFILE *file) { return file->size; }

bool FSFILE_SetSize(FSFILE *file, int64_t size) { return R_SUCCEEDED(fsFileSetSize(&file->handle, size)); }

bool FSFILE_Flush(FSFILE *file)
{
    if (!file) { return false; }

    return R_SUCCEEDED(fsFileFlush(&file->handle));
}

void FSFILE_Close(FSFILE *file)
{
    // Check
    if (!file) { return; }

    // Close the handle.
    fsFileClose(&file->handle);

    // Free the data
    free(file);
}

void FSFILE_Finalize(FSFILE *file)
{
    // Start with flush.
    FSFILE_Flush(file);

    // Set the size according to what the file internally thinks it is. Note: I don't like this. Might need to revise. Serves
    // its purpose though.
    FSFILE_SetSize(file, file->offset);

    // Close.
    FSFILE_Close(file);
}