#pragma once
#include <switch.h>

// This is a bare-minimum wrapper around the FsFile functions to make them
// easier to work with.
typedef struct FSFILE FSFILE;

// This stuff is named like this to avoid conflicts with libnx.

/// @brief Returns if the path passed can be opened for reading from the
/// filesystem passed.
/// @param filesystem Filesystem to check.
/// @param path Path to the file to check.
bool FSFILE_Exists(FsFileSystem *filesystem, const char *path);

/// @brief Attempts to the delete the file passed from the filesystem.
/// @param filesystem Filesystem to delete from.
/// @param path Path to the file to delete.
/// @return True on success. False on failure.
bool FSFILE_Delete(FsFileSystem *filesystem, const char *path);

/// @brief Attempts to open the path passed for writing. Returns NULL on
/// failure.
/// @param filesystem Filesystem to open the file from.
/// @param filePath Path to open.
FSFILE *FSFILE_Open(FsFileSystem *filesystem, const char *path);

/// @brief Attempts to write the buffer to the FSFILE passed.
/// @param file File to write to.FSFILE *file,
/// @param buffer Buffer to write.
/// @param size Size of the buffer to write.
/// @return Bytes written on success. -1 on failure.
ssize_t FSFILE_Write(FSFILE *file, void *buffer, size_t size);

/// @brief Flushes the file passed.
/// @param file File to flush.
bool FSFILE_Flush(FSFILE *file);

/// @brief Closes the file passed.
/// @param file File to close.
void FSFILE_Close(FSFILE *file);
