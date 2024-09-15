# PNGShot
A sysmodule I wrote, _and thought I lost_, in 2020 (or 2021?) that captures screenshots as PNG's instead of JPEGs on Nintendo Switch.

## Why?
Back when bitmap-printer was released, a buddy told me it was cool, but the screenshots were too large. I went to work researching libpng and scouring libnx's headers to figure out how to capture the screenshot data only to be faced with the IPC calls requiring debug mode to work. Because of this, and not wanting to cause possible drama, I decided to keep it between us. Years later while searching through backups I had, I found the source for PNGShot (originally just named `pngscr`), which I thought I lost long ago. Why let it go to waste?

## Notes
While I feel extremely odd about this, you **WILL** need the exefs_patches for [bitmap-printer](https://github.com/HookedBehemoth/bitmap-printer) for PNGShot to function for the time being. I hope the author(s) are OK with this. If not, they are more than free to contact me about it. Screenshots are exported to `SDMC:/switch/PNGShot/[year][month][day][hour][minutes][seconds].png` when the capture button is pressed. They are standard RGB, containing no alpha to minimize size. They are captured using the screenshot viLayer, so they _do_ contain the same watermarks as normal screenshots. This can easily be changed by editing line 99 for the "experimental" build or 170 of `capture.c`  in PNGShot's source.

## Big thanks
* [HookedBehemoth](https://github.com/HookedBehemoth) for [bitmap-printer](https://github.com/HookedBehemoth/bitmap-printer) and its patches.
* [Impeeza](https://github.com/impeeza) for enhancing the makefile and readme to be more descriptive and easier to use. Thank you.

## Building
Requires:
* The System Wide Packages:
  * git
  * make
  * zip

* A working [DevKitPro](https://devkitpro.org/wiki/devkitPro_pacman) environment with the following packages:
  * devkitA64
  * switch-tools
  * libnx
  * switch-libpng
  * switch-zlib
  * hactool

For example, in an MSYS MinGW64 environment you would execute the following commands:

```
pacman -Syuu --needed --noconfirm git make zip devkitA64 switch-tools libnx switch-libpng switch-zlib hactool
cd ~
rm -rf ~/PNGShot
git clone --recursive https://github.com/J-D-K/PNGShot
cd ~/PNGShot
make -j$(nproc)
```
A folder named `dist` will be created in the root of the repository containing a zip file ready to be installed on your sytem and also a set of subfolders with the files needed.
