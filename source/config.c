#include "config.h"

#include "FSFILE.h"

#include <json-c/json.h>
#include <malloc.h>
#include <stdbool.h>
#include <string.h>
#include <switch.h>

/// @brief Whether or not to allow jpeg captures. False by default.
static bool allowJpegs = false;

/// @brief The compression level. 4 by default.
static int compressionLevel = 4;

void config_load(void)
{
    // Config path.
    static const char *CONFIG_PATH           = "/config/PNGShot/config.json";
    static const char *KEY_ALLOW_JPEG        = "AllowJPEGs";
    static const char *KEY_COMPRESSION_LEVEL = "CompressionLevel";

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
        const bool keyJpegs       = strcmp(key, KEY_ALLOW_JPEG) == 0;
        const bool keyCompression = !keyJpegs && strcmp(key, KEY_COMPRESSION_LEVEL) == 0;

        if (keyJpegs) { allowJpegs = json_object_get_boolean(value); }
        else if (keyCompression) { compressionLevel = json_object_get_uint64(value); }
    }

    // Take care of funny business.
    if (compressionLevel > 9) { compressionLevel = 4; }

cleanup:
    if (config) { FSFILE_Close(config); }
    if (configBuffer) { free(configBuffer); }
    if (configJson) { json_object_put(configJson); }
    fsFsClose(&sdmc);
}

bool config_allow_jpeg(void) { return allowJpegs; }

int config_compression_level(void) { return compressionLevel; }