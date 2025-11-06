#include "fsdir.h"

#include "FSFILE.h"

#include <string.h>

bool directory_exists(FsFileSystem *filesystem, const char *path)
{
    // Directory flags.
    static const uint32_t DIR_OPEN_FLAGS = FsDirOpenMode_ReadDirs | FsDirOpenMode_ReadFiles;

    FsDir testDir;
    const bool openFailed = R_FAILED(fsFsOpenDirectory(filesystem, path, DIR_OPEN_FLAGS, &testDir));
    if (openFailed) { return false; }

    fsDirClose(&testDir);
    return true;
}

bool create_directory_recursively(FsFileSystem *filesystem, const char *path)
{
    // Check this first to ensure we weren't passed an empty path or only /
    const size_t pathLength = strlen(path);
    if (pathLength <= 1) { return false; }

    // This buffer is used for the sub-path.
    char subPath[FS_MAX_PATH];

    // This makes the assumption the path starts with a slash. It should to be valid.
    const char *pathOffset = path;
    while ((pathOffset = strchr(pathOffset + 1, '/')))
    {
        // subPath.
        strncpy(subPath, path, pathOffset - path);

        // Try to create it.
        const bool exists      = directory_exists(filesystem, subPath);
        const bool createError = !exists && R_FAILED(fsFsCreateDirectory(filesystem, subPath));
        if (!exists && createError) { return false; }
    }

    return true;
}