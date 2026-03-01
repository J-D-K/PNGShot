#include "jpeg.h"

#include "FSFILE.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

// Defined at bottom.

/// @brief Returns the absolute time difference between the two timestamps passed.
/// @param stampA First stamp to compare.
/// @param stampB Second stamp to compare.
static inline uint64_t absolute_time_difference(uint64_t stampA, uint64_t stampB);

bool jpeg_delete_capture(FsFileSystem *albumDir, uint64_t timestamp)
{
    // Get the current local time of the system.
    struct tm localTime = *localtime((const time_t *)&timestamp);

    // This is what we're using in the end.
    uint64_t lowestDelta         = UINT64_MAX;
    char targetJpeg[FS_MAX_PATH] = {0};

    // Construct the path.
    char targetPath[FS_MAX_PATH] = {0};
    snprintf(targetPath, FS_MAX_PATH, "/%04d/%02d/%02d", localTime.tm_year + 1900, localTime.tm_mon + 1, localTime.tm_mday);

    // Try to open the target directory.
    FsDir targetDir;
    const bool openError = R_FAILED(fsFsOpenDirectory(albumDir, targetPath, FsDirOpenMode_ReadFiles, &targetDir));
    if (openError) { return false; }

    // Loop through the directory.
    int64_t readCount;
    FsDirectoryEntry entry;
    while (R_SUCCEEDED(fsDirRead(&targetDir, &readCount, 1, &entry)) && readCount > 0)
    {
        // Get where the extension begins. If the extension isn't jpg, don't bother.
        const char *extension = strchr(entry.name, '.');
        if (!extension) { continue; }

        // Ensure the extension is jpg before wasting time on it.
        ++extension;
        if (strcmp(extension, "jpg") != 0) { continue; }

        // Construct our final target.
        char stampPath[FS_MAX_PATH] = {0};
        snprintf(stampPath, FS_MAX_PATH, "%s/%s", targetPath, entry.name);

        // Read the timestamp.
        FsTimeStampRaw rawStamp;
        const bool stampError = R_FAILED(fsFsGetFileTimeStampRaw(albumDir, stampPath, &rawStamp));
        if (stampError) { continue; }

        const uint64_t delta = absolute_time_difference(timestamp, rawStamp.created);
        if (delta < lowestDelta)
        {
            lowestDelta = delta;
            // To do: Look into a different solution for this. Difficult, but not impossible.
            strncpy(targetJpeg, stampPath, FS_MAX_PATH);
        }
    }

    // Close the handle.
    fsDirClose(&targetDir);
    if (lowestDelta != UINT64_MAX) { FSFILE_Delete(albumDir, targetJpeg); }

    return true;
}

static inline uint64_t absolute_time_difference(uint64_t stampA, uint64_t stampB)
{
    return stampA > stampB ? stampA - stampB : stampB - stampA;
}