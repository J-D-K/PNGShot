#pragma once

/// @brief (Attempts to) load the config json.
/// @return True on success. False on failure.
void config_load(void);

/// @brief Returns whether or not the allow the JPEG captures to exist.
/// @return True if jpegs are to be allowed. False if not.
bool config_allow_jpeg(void);

/// @brief Returns the compression level read from config.
int config_compression_level(void);