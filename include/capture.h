#pragma once
#include <switch.h>

// Normally I try to avoid globals like this, but screw it.
extern bool g_NoJpeg;

void captureScreenshot(FsFileSystem *filesystem, const char *filePath);
