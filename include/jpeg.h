#pragma once
#include <stdbool.h>
#include <switch.h>

/// @brief Checks for the flag on the SD card for whether or not to delete JPEG captures.
void jpeg_check_for_flag();

/// @brief Returns whether or not the internal flags calls for deleting JPEG captures
bool jpeg_needs_deletion();

/// @brief Deletes the jpep capture on the SD card closest to the timestamp passed.
/// @param albumDir Filesystem pointing to the album directory.
/// @param timestamp Timestamp to use as reference.
/// @return True on success. False on failure.
bool jpeg_delete_capture(FsFileSystem *albumDir, uint64_t timestamp);
