# PNGShot Configuration

If you would like to change the settings of PNGShot, create a file named `config.json` and place it in the folder `sdmc:/config/PNGShot` on your Switch's SD card.

Here is an example configuration file using the default values:
```json
{
    "AllowJPEGs": false,
    "CompressionLevel": 4
}
```
### Config Keys

* **AllowJPEGs**: Whether or not PNGShot should allow JPEG captures to be saved. When set to `false`, the default JPEG captures of the Switch are deleted. When set to true, they are ignored and not touched. The default setting for this is `false`.

* **CompressionLevel**: The compression level used when saving a screenshot. This can range from `0` (uncompressed) to `9` (maximum). Any value outside of this range will be corrected to the default. The default value of this is `4`.