#pragma once
#include "screenshot.h"

#include <stdbool.h>
#include <stdint.h>
#include <switch.h>

/// @brief Opens the screenshot stream. Returns true on success.
inline bool capssc_open_stream()
{
    // Time out for capture.
    static const int64_t CAPTURE_TIMEOUT = 1e+8;

    // Needed for capture, but kind of irrelevant?
    uint64_t size;
    uint64_t width;
    uint64_t height;
    return R_SUCCEEDED(capsscOpenRawScreenShotReadStream(&size, &width, &height, ViLayerStack_Screenshot, CAPTURE_TIMEOUT));
}

/// @brief Reads the row or chunk from the screenshot stream using the row offset passed.
/// @param buffer Buffer to read into.
/// @param rowOffset Offset to read a row of pixels from.
/// @return True on success. False on failure.
inline bool capssc_read_row(void *buffer, int rowOffset)
{
    // This is always the same.
    static const size_t ROW_SIZE = SCREENSHOT_WIDTH * 4;

    // Read the chunk at the offset.
    uint64_t bytesRead;
    const bool rowRead = R_SUCCEEDED(capsscReadRawScreenShotReadStream(&bytesRead, buffer, ROW_SIZE, rowOffset * ROW_SIZE));
    return rowRead && bytesRead == ROW_SIZE;
}