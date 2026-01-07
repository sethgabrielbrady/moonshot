#ifndef UTILS_H
#define UTILS_H

#include <math.h>
#include <stdlib.h>
#include "constants.h"

// =============================================================================
// Time Functions
// =============================================================================

static inline float get_time_s(void) {
    return (float)((double)get_ticks_us() / 1000000.0);
}

// =============================================================================
// Math Utilities
// =============================================================================

static inline float clampf(float val, float min_val, float max_val) {
    return fmaxf(fminf(val, max_val), min_val);
}

static inline float normalize_angle(float angle) {
    while (angle > T3D_PI) angle -= TWO_PI;
    while (angle < -T3D_PI) angle += TWO_PI;
    return angle;
}

static inline float randomize_float(float min, float max) {
    return min + ((float)rand() / (float)RAND_MAX) * (max - min);
}

// =============================================================================
// Fast Math
// =============================================================================

static inline float fast_inv_sqrt(float x) {
    // Quake III algorithm with union to avoid strict-aliasing issues
    union {
        float f;
        int32_t i;
    } conv;

    float halfx = 0.5f * x;
    conv.f = x;
    conv.i = 0x5f3759df - (conv.i >> 1);
    conv.f = conv.f * (1.5f - (halfx * conv.f * conv.f));
    return conv.f;
}

// =============================================================================
// Compass Direction (for debug)
// =============================================================================

const char* get_compass_direction(float angle);

// =============================================================================
// Color Utilities
// =============================================================================

// static inline uint32_t color_to_packed32(color_t color) {
//     return ((uint32_t)color.r << 24) |
//            ((uint32_t)color.g << 16) |
//            ((uint32_t)color.b << 8) |
//            (uint32_t)color.a;
// }

#endif // UTILS_H
