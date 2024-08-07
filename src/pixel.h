#pragma once
#include <stdint.h>

typedef union {
    uint32_t u32;
    uint8_t u8[4];
    struct { uint8_t b, g, r, a; };
} pixel_t;
