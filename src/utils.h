#ifndef UTILS_H
#define UTILS_H

#include <libdragon.h>

// =============================================================================
// Time Functions
// =============================================================================

float get_time_s(void);

// =============================================================================
// Math Utilities
// =============================================================================

float clampf(float val, float min_val, float max_val);
float normalize_angle(float angle);
float randomize_float(float min, float max);

// =============================================================================
// Fast Math
// =============================================================================

float fast_inv_sqrt(float x);

// =============================================================================
// Compass Direction (for debug)
// =============================================================================

const char* get_compass_direction(float angle);

// =============================================================================
// Rumble Pak Support
// =============================================================================

void init_rumble(void);
void trigger_rumble(float duration);
void update_rumble(float delta_time);
void stop_rumble(void);


#endif // UTILS_H