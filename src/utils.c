#include "utils.h"
#include "constants.h"
#include <math.h>
#include <stdlib.h>
#include <stdint.h>

// =============================================================================
// Time Functions
// =============================================================================

float get_time_s(void) {
    return (float)((double)get_ticks_us() / 1000000.0);
}

// =============================================================================
// Math Utilities
// =============================================================================

float clampf(float val, float min_val, float max_val) {
    return fmaxf(fminf(val, max_val), min_val);
}

float normalize_angle(float angle) {
    while (angle > T3D_PI) angle -= TWO_PI;
    while (angle < -T3D_PI) angle += TWO_PI;
    return angle;
}

float randomize_float(float min, float max) {
    return min + ((float)rand() / (float)RAND_MAX) * (max - min);
}

// =============================================================================
// Fast Math
// =============================================================================

float fast_inv_sqrt(float x) {
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

const char* get_compass_direction(float angle) {
    while (angle < 0) angle += TWO_PI;
    while (angle >= TWO_PI) angle -= TWO_PI;

    if (angle < T3D_PI / 8.0f || angle >= 15.0f * T3D_PI / 8.0f) return "N";
    if (angle < 3.0f * T3D_PI / 8.0f) return "NE";
    if (angle < 5.0f * T3D_PI / 8.0f) return "E";
    if (angle < 7.0f * T3D_PI / 8.0f) return "SE";
    if (angle < 9.0f * T3D_PI / 8.0f) return "S";
    if (angle < 11.0f * T3D_PI / 8.0f) return "SW";
    if (angle < 13.0f * T3D_PI / 8.0f) return "W";
    return "NW";
}