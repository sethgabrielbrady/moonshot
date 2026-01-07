#ifndef CAMERA_H
#define CAMERA_H

#include <libdragon.h>
#include <t3d/t3d.h>
#include <t3d/t3dmath.h>
#include "types.h"

// =============================================================================
// Camera Structure (defined here, not in types.h)
// =============================================================================

typedef struct {
    T3DVec3 position;
    T3DVec3 target;
    float rotation_y;
} Camera;

// =============================================================================
// Global Camera Instance
// =============================================================================

extern Camera camera;

// =============================================================================
// Cursor Look Direction (used by FPS mode)
// =============================================================================

extern T3DVec3 cursor_look_direction;

// =============================================================================
// Screen Shake
// =============================================================================

extern float screen_shake_timer;
extern float screen_shake_intensity;
extern bool other_shake_enabled;

void trigger_screen_shake(float intensity, float duration);
void update_screen_shake(float delta_time);

// =============================================================================
// Camera Zoom
// =============================================================================

extern bool camera_zoom_to_station;
extern float camera_zoom_timer;
extern T3DVec3 camera_zoom_target;

void trigger_camera_zoom(T3DVec3 target, float duration);

// =============================================================================
// Camera Functions
// =============================================================================

void reset_cam_yaw(float *cam_yaw);
void teleport_to_position(float x, float z, float *cam_yaw, T3DVec3 *cursor_position);
void update_camera(T3DViewport *viewport, float cam_yaw, float delta_time, 
                   T3DVec3 cursor_position, bool fps_mode, Entity *cursor_entity);

#endif // CAMERA_H
