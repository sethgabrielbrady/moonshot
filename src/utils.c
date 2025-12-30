#include "utils.h"

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

