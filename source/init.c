#include "init.h"

bool init_open_album_directory(FsFileSystem *albumOut)
{
    bool opened = R_SUCCEEDED(fsOpenImageDirectoryFileSystem(albumOut, FsImageDirectoryId_Sd));
    if (!opened) { opened = R_SUCCEEDED(fsOpenImageDirectoryFileSystem(albumOut, FsImageDirectoryId_Nand)); }

    return opened;
}

bool init_create_pngshot_directory(FsFileSystem *albumDir)
{
    // Path.
    static const char *PNGSHOT_DIR       = "/PNGs";
    static const uint32_t DIR_OPEN_FLAGS = FsDirOpenMode_ReadDirs | FsDirOpenMode_ReadFiles;

    // Check if it already exists.
    FsDir handle;
    const bool opened = R_SUCCEEDED(fsFsOpenDirectory(albumDir, PNGSHOT_DIR, DIR_OPEN_FLAGS, &handle));
    // It exists already. Just return true.
    if (opened)
    {
        fsDirClose(&handle);
        return true;
    }

    // If we're here, we need to create it.
    const bool createError = R_FAILED(fsFsCreateDirectory(albumDir, PNGSHOT_DIR));
    if (createError) { return false; }

    return true;
}