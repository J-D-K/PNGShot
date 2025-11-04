#include "init.h"

bool init_open_album_directory(FsFileSystem *albumOut)
{
    bool openError = R_FAILED(fsOpenImageDirectoryFileSystem(albumOut, FsImageDirectoryId_Sd));
    if (openError) { openError = R_FAILED(fsOpenImageDirectoryFileSystem(albumOut, FsImageDirectoryId_Nand)); }

    // This is weird, but trust me.
    return openError == false;
}

bool init_create_pngshot_directory(FsFileSystem *albumDir)
{
    // Path.
    static const char *PNGSHOT_DIR = "/PNGs";

    // Needed flags.
    static const uint32_t DIR_FLAGS = FsDirOpenMode_ReadFiles | FsDirOpenMode_ReadDirs;

    // Check if it already exists.
    FsDir handle;
    const bool opened = R_SUCCEEDED(fsFsOpenDirectory(albumDir, PNGSHOT_DIR, DIR_FLAGS, &handle));
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