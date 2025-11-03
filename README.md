# PNGShot
<p align="center">
  <a href="https://github.com/J-D-K/PNGShot">
    <img alt="Language" src="https://img.shields.io/github/languages/top/J-D-K/PNGShot?style=for-the-badge">
  </a>
  <a href="https://github.com/J-D-K/PNGShot/stargazers">
    <img alt="GitHub stars" src="https://img.shields.io/github/stars/J-D-K/PNGShot?style=for-the-badge">
  </a>
  <a href="https://github.com/J-D-K/PNGShot/network/members">
    <img alt="GitHub forks" src="https://img.shields.io/github/forks/J-D-K/PNGShot?style=for-the-badge">
  </a>
  <a href="https://github.com/J-D-K/PNGShot/releases">
    <img alt="GitHub downloads" src="https://img.shields.io/github/downloads/J-D-K/PNGShot/total?style=for-the-badge">
  </a>
  <a href="https://github.com/J-D-K/PNGShot/releases/latest">
    <img alt="GitHub release" src="https://img.shields.io/github/v/release/J-D-K/PNGShot?style=for-the-badge">
  </a>
</p>

---

## Overview
**PNGShot** is a lightweight homebrew **sysmodule** for the **Nintendo Switch**, written entirely in **C**, that captures screenshots and saves them as **PNG** files instead of the system’s default **JPEG** format.

Using PNG provides lossless compression that preserves the quality of the capture while keeping file sizes smaller.

## Features
- Captures system screenshots as **lossless PNG** images
- Optional compatibility mode to allow both PNG and JPEG captures  
- Simple SD card–based configuration  
- Low-overhead sysmodule design written in pure C  
- Highly stable

## Installation and usage
1. Download the latest release from the [Releases page](https://github.com/J-D-K/PNGShot/releases/).  
2. Extract the contents of the ZIP archive to the **root** of your Switch’s SD card.  
3. ***(Optional)*** To keep system JPEG captures, create an empty file named `sdmc:/config/PNGShot/allow_jpegs`
4. If a future Switch firmware update causes compatibility issues, check the [patches directory](https://github.com/J-D-K/PNGShot/tree/master/exefs_patches/vi_patches) for updated patches. In most cases, if PNGShot breaks after a firmware update, this is the cause. The sysmodule code itself is extremely stable.

## Building from source
### Requirements
* Ensure you have the following tools installed:
  ```
  git
  make
  zip
  ```
* A working [DevKitPro](https://devkitpro.org/wiki/devkitPro_pacman) installation with the following packages:
  ```
  devkitA64
  switch-tools
  libnx
  switch-libpng
  switch-zlib
  ```
* You will also need to build hactool.
  
### Building instructions
* Clone PNGShot's repository with the following command:
  ```
  git clone https://github.com/J-D-K/PNGShot.git
  ```
* Open the cloned repository and run the following command:
  ```
  make -j
  ```
* Once PNGShot is built, you will have a folder named `dist` in the root of your local copy of the repository. Copy the contents to your SD card along with the patches included from cloning the repo.

## Big Thanks
* Impeeza for enhancing the makefile and the basis for the patch generating script.