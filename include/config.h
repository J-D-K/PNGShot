#pragma once

// clang-format off
typedef struct
{
    bool allowJpegs;
    bool webpLossless;
    int pngLevel;
    int webpLevel;
} ConfigStruct;
// clang-format on

/// @brief (Attempts to) load the config json.
/// @return True on success. False on failure.
void config_load(void);

/// @brief Returns const pointer to the static config struct.
const ConfigStruct *config_get(void);
