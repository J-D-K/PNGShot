#include "FSFILE.h"

#include <malloc.h>
#include <stdbool.h>

// clang-format off
struct FSFILE
{
    FsFile handle;
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

FSFILE *FSFILE_Open(FsFileSystem *filesystem, const char *path)
{
    // Both of these are zero to start since we don't have any idea what the ending PNG size will be.
    static const int64_t CREATING_SIZE    = 0;
    static const uint32_t CREATING_OPTION = 0;

    // Check if it exists. If it doesn't create it.
    const bool exists  = FSFILE_Exists(filesystem, path);
    const bool deleted = exists && FSFILE_Delete(filesystem, path);
    if (exists && !deleted) { goto cleanup; }

    // Try to create it. If we can't cleanup and return NULL.
    const bool createError = R_FAILED(fsFsCreateFile(filesystem, path, CREATING_SIZE, CREATING_OPTION));
    if (createError) { goto cleanup; }

    // Allocate our return data.
    FSFILE *fsfile = malloc(sizeof(FSFILE));
    if (!fsfile) { goto cleanup; }

    // Attempt to open the file for writing.
    const bool openError = R_FAILED(fsFsOpenFile(filesystem, path, FsOpenMode_Write, &fsfile->handle));
    if (openError) { goto cleanup; }

    // Set the offset to 0.
    fsfile->offset = 0;

    // Return the data.
    return fsfile;

cleanup:
    // If this actually exists, the only error that would send it here is it failing to open, so no need to worry about closing
    // the handle.
    if (fsfile) { free(fsfile); }

    return NULL;
}

ssize_t FSFILE_Write(FSFILE *file, void *buffer, size_t size)
{
    // Do not continue if we're not passed valid data.
    if (!file || !buffer) { return -1; }

    // Update the size of the file.
    const int64_t newSize = file->offset + size;
    const bool sizeError  = R_FAILED(fsFileSetSize(&file->handle, newSize));
    if (sizeError) { return -1; }

    // Attempt to write the data.
    const bool writeError = R_FAILED(fsFileWrite(&file->handle, file->offset, buffer, size, FsWriteOption_None));
    if (writeError) { return -1; }

    // Update the offset and return the size.
    file->offset += size;

    return size;
}

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
