#ifndef UTILS_H
#define UTILS_H

#include <math.h>
#include <stdlib.h>
#include "constants.h"

static inline float get_time_s(void) {
    return (float)((double)get_ticks_us() / 1000000.0);
}

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

const char* get_compass_direction(float angle);


#endif
