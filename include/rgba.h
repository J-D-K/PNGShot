#pragma once
#include "screenshot.h"

#include <arm_neon.h>
#include <stdint.h>

inline void rgba_strip_alpha(uint8_t *row)
{
    int i = 0;
    int j = 0;

    for (; i <= (SCREENSHOT_WIDTH - 16) * 4; i += 64, j += 48)
    {
        uint8x16x4_t rgba = vld4q_u8(row + i);
        uint8x16x3_t rgb  = {{rgba.val[0], rgba.val[1], rgba.val[2]}};
        vst3q_u8(row + j, rgb);
    }

    for (; i < SCREENSHOT_WIDTH * 4; i += 4, j += 3)
    {
        row[j]     = row[i];
        row[j + 1] = row[i + 1];
        row[j + 2] = row[i + 2];
    }
}