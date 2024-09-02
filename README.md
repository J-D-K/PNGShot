# PNGShot
A sysmodule I wrote, _and thought I lost_, in 2020 (or 2021?) that captures screenshots as PNG's instead of JPEGs.

## Why?
Back when bitmap-printer was released, a buddy said it was cool, but said it'd be better if it exported PNG's. I went to work researching libpng and scouring libnx's headers to figure out how to capture the screenshot data only to be faced with the IPC calls requiring debug mode to work at all. Because of this, and not wanting to cause possible drama, I decided to keep it between us. Years later while searching through backups I had, I found the source for PNGShot, which I thought I lost long ago. Why let it go to waste?

## Notes
While I feel extremely odd about this, you **WILL** need the exefs_patches for [bitmap-printer](https://github.com/HookedBehemoth/bitmap-printer) for PNGShot to function. I hope the author(s) are OK with this. If not, they are more than free to contact me to have this removed. Screenshots are exported to `SDMC:/switch/PNGShot/[year][month][day][hour][minutes][seconds].png` when the capture button is pressed. They are standard RGB, containing no alpha to minimize size. They are captured using the screenshot viLayer, so they _do_ contain the same watermarks as normal screenshots. This can easily be changed by editing one line in PNGShot's source.

## Big thanks
[HookedBehemoth](https://github.com/HookedBehemoth) for [bitmap-printer](https://github.com/HookedBehemoth/bitmap-printer) and its patches.

## Building
Requires:
* devkitpro
* switch-libpng
* switch-zlib