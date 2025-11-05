#pragma once
#include <switch.h>

/// @brief Captures the current screenshot stream and exports it to a PNG.
/// @param albumDir Filesystem pointing to the album directory.
void png_capture(FsFileSystem *albumDir);
