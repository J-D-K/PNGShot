#pragma once
#include <switch.h>

/// @brief Returns if the directory path passed exists in the filesystem passed.
/// @param filesystem Filesystem to check.
/// @param path Path of the directory to check.
bool directory_exists(FsFileSystem *filesystem, const char *path);

/// @brief Creates the directory path passed recursively in the filesystem passed.
/// @param filesystem Filesystem to use.
/// @param path Path of the directory to create.
bool create_directory_recursively(FsFileSystem *filesystem, const char *path);