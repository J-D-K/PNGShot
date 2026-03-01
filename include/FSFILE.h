#pragma once
#include <stdbool.h>
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

/// @brief Attempts to open the path passed for reading.
/// @param filesystem Filesystem on which the file resides.
/// @param path Path to open.
/// @return FSFILE upon success. NULL on failure.
FSFILE *FSFILE_OpenRead(FsFileSystem *filesystem, const char *path);

/// @brief Attempts to open the path passed for writing.
/// @param filesystem Filesystem on which the file will be created and written to.
/// @param path Path to open.
/// @param size Size to create with.
/// @return FSFILE on success. NULL on failure.
FSFILE *FSFILE_OpenWrite(FsFileSystem *filesystem, const char *path, int64_t size);

/// @brief Reads from the file passed.
/// @param file File to read from.
/// @param buffer Buffer to read to.
/// @param size Size of the buffer to read to.
/// @return
ssize_t FSFILE_Read(FSFILE *file, void *buffer, size_t size);

/// @brief Attempts to write the buffer to the FSFILE passed.
/// @param file File to write to.FSFILE *file,
/// @param buffer Buffer to write.
/// @param size Size of the buffer to write.
/// @return Bytes written on success. -1 on failure.
ssize_t FSFILE_Write(FSFILE *file, const void *buffer, size_t size);

/// @brief Returns the size of the file.
/// @param file File to get the size of.
/// @return Size of the file on success. -1 on failure.
ssize_t FSFILE_GetSize(FSFILE *file);

/// @brief Sets the size of the file.
/// @param file File to set the size of.
/// @param size Size to set the file to.
/// @return True on success. False on failure.
bool FSFILE_SetSize(FSFILE *file, int64_t size);

/// @brief Flushes the file passed.
/// @param file File to flush.
bool FSFILE_Flush(FSFILE *file);

/// @brief Closes the file passed.
/// @param file File to close.
void FSFILE_Close(FSFILE *file);

/// @brief Finalizes the file if it's open for writing.
/// @param file File to finalize.
void FSFILE_Finalize(FSFILE *file);
