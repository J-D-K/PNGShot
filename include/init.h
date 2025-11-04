#pragma once
#include <stdbool.h>
#include <switch.h>

/// @brief Attempts to open the album filesystem on SD, falling back to NAND if needed.
/// @param albumOut Filesystem to store the handle to.
/// @return True on success. False on failure.
bool init_open_album_directory(FsFileSystem *albumOut);

/// @brief Attempts to create the PNGShot directory.
/// @param albumDir Album filesystem.
/// @return True on success. False on failure.
bool init_create_pngshot_directory(FsFileSystem *albumDir);