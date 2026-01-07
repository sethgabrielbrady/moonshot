#ifndef CAMERA_H
#define CAMERA_H

#include "types.h"
#include "utils.h"

extern Camera camera;

extern float camera_zoom_timer;
extern bool camera_zoom_to_station;

void reset_cam_yaw(float *cam_yaw);
void teleport_to_position(float x, float z, float *cam_yaw, T3DVec3 *cursor_position);
void update_camera(T3DViewport *viewport, float cam_yaw, float delta_time, T3DVec3 cursor_position, bool fps_mode, Entity *cursor_entity);
void update_screen_shake(float delta_time);
extern void trigger_screen_shake(float intensity, float duration);
void trigger_camera_zoom(T3DVec3 target, float duration);

#endif