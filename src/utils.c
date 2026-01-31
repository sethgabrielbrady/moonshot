#include "utils.h"
#include "constants.h"
#include <math.h>
#include <stdlib.h>
#include <stdint.h>





// =============================================================================
// Rumble Pak Support
// =============================================================================

static float rumble_timer = 0.0f;
static bool rumble_available = false;
static bool rumble_checked = false;

void init_rumble(void) {
    rumble_checked = true;
    rumble_available = (joypad_get_accessory_type(JOYPAD_PORT_1) == JOYPAD_ACCESSORY_TYPE_RUMBLE_PAK);
}

void trigger_rumble(float duration) {
    if (!rumble_checked) init_rumble();
    if (!rumble_available) return;

    // Only extend rumble if new duration is longer
    if (duration > rumble_timer) {
        rumble_timer = duration;
        joypad_set_rumble_active(JOYPAD_PORT_1, true);
    }
}

void update_rumble(float delta_time) {
    if (!rumble_available || rumble_timer <= 0.0f) return;

    rumble_timer -= delta_time;
    if (rumble_timer <= 0.0f) {
        rumble_timer = 0.0f;
        joypad_set_rumble_active(JOYPAD_PORT_1, false);
    }
}

void stop_rumble(void) {
    if (!rumble_available) return;
    rumble_timer = 0.0f;
    joypad_set_rumble_active(JOYPAD_PORT_1, false);
}


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