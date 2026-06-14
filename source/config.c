#include "config.h"

#include "FSFILE.h"

#include <json-c/json.h>
#include <malloc.h>
#include <stdbool.h>
#include <string.h>
#include <switch.h>

/// @brief Single config struct to contain the config better.
static ConfigStruct s_configStruct;

void config_load(void)
{
    // Set defaults.
    s_configStruct.allowJpegs   = false;
    s_configStruct.webpLossless = false;
    s_configStruct.pngLevel     = 4;
    s_configStruct.webpLevel    = 2;

    // Config path.
    static const char *CONFIG_PATH       = "/config/PNGShot/config.json";
    static const char *KEY_ALLOW_JPEG    = "AllowJPEGs";
    static const char *KEY_LOSSLESS_WEBP = "LosslessWebP";
    static const char *KEY_PNG_LEVEL     = "PNGLevel";
    static const char *KEY_WEBP_LEVEL    = "WebpLevel";

    // Open the sdmc.
    FsFileSystem sdmc     = {0};
    const bool sdmcOpened = R_SUCCEEDED(fsOpenSdCardFileSystem(&sdmc));
    if (!sdmcOpened) { goto cleanup; }

    // If the config doesn't exist, just bail and roll with defaults.
    if (!FSFILE_Exists(&sdmc, CONFIG_PATH)) { goto cleanup; }

    // We're going to read this to a temp buffer because I don't want to use stdio.
    FSFILE *config = FSFILE_OpenRead(&sdmc, CONFIG_PATH);
    if (!config) { goto cleanup; }

    // Buffer.
    const int64_t configLength = FSFILE_GetSize(config);
    char *configBuffer         = malloc(configLength);
    if (!configBuffer) { goto cleanup; }

    // Read.
    const bool read = FSFILE_Read(config, configBuffer, configLength) == configLength;
    if (!read) { goto cleanup; }

    // Parse and read json.
    json_object *configJson = json_tokener_parse(configBuffer);
    if (!configJson) { goto cleanup; }

    // Iterate through json.
    struct json_object_iterator current = json_object_iter_begin(configJson);
    struct json_object_iterator end     = json_object_iter_end(configJson);
    for (; !json_object_iter_equal(&current, &end); json_object_iter_next(&current))
    {
        // Makes stuff easier to read.
        const char *key          = json_object_iter_peek_name(&current);
        const json_object *value = json_object_iter_peek_value(&current);

        // Key eval.
        const bool keyJpegs        = strcmp(key, KEY_ALLOW_JPEG) == 0;
        const bool keyCompression  = !keyJpegs && strcmp(key, KEY_PNG_LEVEL) == 0;
        const bool keyLosslessWebp = !keyCompression && strcmp(key, KEY_LOSSLESS_WEBP) == 0;
        const bool keyWebpLevel    = !keyLosslessWebp && strcmp(key, KEY_WEBP_LEVEL) == 0;

        if (keyJpegs) { s_configStruct.allowJpegs = json_object_get_boolean(value); }
        else if (keyCompression) { s_configStruct.pngLevel = json_object_get_uint64(value); }
        else if (keyLosslessWebp) { s_configStruct.webpLossless = json_object_get_boolean(value); }
        else if (keyWebpLevel) { s_configStruct.webpLevel = json_object_get_uint64(value); }
    }

    // Take care of funny business.
    if (s_configStruct.pngLevel > 9) { s_configStruct.pngLevel = 4; }
    if (s_configStruct.webpLevel > 6) { s_configStruct.webpLevel = 2; }

cleanup:
    if (config) { FSFILE_Close(config); }
    if (configBuffer) { free(configBuffer); }
    if (configJson) { json_object_put(configJson); }
    fsFsClose(&sdmc);
}

const ConfigStruct *config_get(void) { return &s_configStruct; }