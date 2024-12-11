#include "fsfile.h"
#include <malloc.h>

bool FSFILEExists(FsFileSystem *filesystem, const char *filePath)
{
    FsFile fileHandle;
    Result fsError = fsFsOpenFile(filesystem, filePath, FsOpenMode_Read, &fileHandle);
    if (R_FAILED(fsError))
    {
        return false;
    }
    fsFileClose(&fileHandle);
    return true;
}

FSFILE *FSFILEOpen(FsFileSystem *filesystem, const char *filePath)
{
    // Try to create file first. We don't know what the ending size will be.
    Result fsError = fsFsCreateFile(filesystem, filePath, 0, 0);
    if (R_FAILED(fsError))
    {
        return NULL;
    }

    // Try to allocate FSFILE to return
    FSFILE *newFile = malloc(sizeof(FSFILE));
    if (newFile == NULL)
    {
        return NULL;
    }

    // Try to open file.
    fsError = fsFsOpenFile(filesystem, filePath, FsOpenMode_Write, &newFile->fileHandle);
    if (R_FAILED(fsError))
    {
        free(newFile);
        return NULL;
    }
    // File should be opened for writing. Set offset to 0, return it.
    newFile->fileOffset = 0;
    return newFile;
}

size_t FSFILEWrite(FSFILE *file, void *buffer, size_t bufferSize)
{
    // Don't go forward if file pointer or buffer are NULL.
    if (file == NULL || buffer == NULL)
    {
        return 0;
    }

    // Resize the file to fit the incoming buffer.
    Result fsError = fsFileSetSize(&file->fileHandle, file->fileOffset + bufferSize);
    if (R_FAILED(fsError))
    {
        return 0;
    }

    // Write the data.
    fsError = fsFileWrite(&file->fileHandle, file->fileOffset, buffer, bufferSize, FsWriteOption_None);
    if (R_FAILED(fsError))
    {
        return 0;
    }
    // Update offset, return bufferSize on assumed success.
    file->fileOffset += bufferSize;
    return bufferSize;
}

void FSFILEFlush(FSFILE *file)
{
    if (file != NULL)
    {
        fsFileFlush(&file->fileHandle);
    }
}

void FSFILEClose(FSFILE *file)
{
    // Return on NULL pointer.
    if (file == NULL)
    {
        return;
    }
    // Close file handle
    fsFileClose(&file->fileHandle);
    free(file);
}
