#ifndef CAMERA_H
#define CAMERA_H

#include "types.h"

extern Camera camera;

void reset_cam_yaw(float *cam_yaw);
void teleport_to_position(float x, float z, float *cam_yaw, T3DVec3 *cursor_position);
void update_camera(T3DViewport *viewport, float cam_yaw, float delta_time, T3DVec3 cursor_position, bool fps_mode, Entity *cursor_entity);

#endif