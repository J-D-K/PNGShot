# PNGShot
A sysmodule I wrote, _and thought I lost_, in 2020 (or 2021?) that captures screenshots as PNG's instead of JPEGs. Besides updating code for the current libnx, ~~this is completely untested and only builds _as far as I know_. Use at your own risk, but there's nothing here that can cause _permanent_ damage...~~ Nevermind, it still works fine.

## Why?
Back when bitmap-printer was released, a buddy said it was cool, but said it'd be better if it exported PNG's. I went to work researching libpng and scouring libnx's headers to figure out how to capture the screenshot data only to be faced with the IPC calls requiring debug mode to work at all. Because of this, and not wanting to cause possible drama, I decided to keep it between us. Years later while searching through backups I had, I found the source for PNGShot, which I thought I lost long ago. Why let it go to waste?

## Notes
While I feel extremely odd about this, you **WILL** need the exefs_patches for [bitmap-printer](https://github.com/HookedBehemoth/bitmap-printer) for PNGShot to function. I hope the author(s) are OK with this. If not, they are more than free to contact me to have this removed. Screenshots are exported to `SDMC:/switch/PNGShot/year-month-day/hour-minutes-seconds.png` when the capture button is pressed. They are standard RGB, containing no alpha to minimize size. They are captured using the screenshot viLayer, so they _do_ contain the same watermarks as normal screenshots. This can easily be changed by editing one line in PNGShot's source.

## Big thanks
[HookedBehemoth](https://github.com/HookedBehemoth) for [bitmap-printer](https://github.com/HookedBehemoth/bitmap-printer) and its patches.

## Building
Requires:
* A working DevKitPro environment, with the packages:
  * devkitA64
  * switch-tools
  * libnx
  * switch-libpng
  * switch-zlib
  * hactool
* And the System Wide Packages:
  * git
  * make
  * zip

By example on a MSYS's MingW64 environment you can execute the next commands:

```
pacman -Syuu --needed --noconfirm git make zip devkitA64 switch-tools libnx switch-libpng switch-zlib hactool
cd ~
rm -rf ~/PNGShot
git clone --recursive https://github.com/J-D-K/PNGShot
cd ~/PNGShot
make -j$(nproc)
```
