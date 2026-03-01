#pragma once
#include <stdbool.h>
#include <switch.h>

/// @brief Deletes the jpep capture on the SD card closest to the timestamp passed.
/// @param albumDir Filesystem pointing to the album directory.
/// @param timestamp Timestamp to use as reference.
/// @return True on success. False on failure.
bool jpeg_delete_capture(FsFileSystem *albumDir, uint64_t timestamp);
