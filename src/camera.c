#include "camera.h"
#include "constants.h"
#include "utils.h"
#include <math.h>

extern T3DVec3 cursor_look_direction;

Camera camera = {
    .position = {{0.0f, 0.0f, 0.0f}},
    .target = {{0.0f, 0.0f, 0.0f}},
    .rotation_y = 0.0f
};

void reset_cam_yaw(float *cam_yaw) {
    *cam_yaw = CAM_ANGLE_YAW;
}

void teleport_to_position(float x, float z, float *cam_yaw, T3DVec3 *cursor_position) {
    camera.target.v[0] = x;
    camera.target.v[2] = z;
    cursor_position->v[0] = x;
    cursor_position->v[2] = z;
    reset_cam_yaw(cam_yaw);
}

void update_camera(T3DViewport *viewport, float cam_yaw, float delta_time, T3DVec3 cursor_position, bool fps_mode, Entity *cursor_entity) {
    if (fps_mode && cursor_entity) {
        float eye_height = 15.0f;
        float look_distance = 100.0f;

        // Camera position at cursor
        camera.position.v[0] = cursor_position.v[0];
        camera.position.v[1] = cursor_position.v[1] + eye_height;
        camera.position.v[2] = cursor_position.v[2];

        // Rotate cursor with stick X axis
        joypad_inputs_t joypad = joypad_get_inputs(JOYPAD_PORT_1);
        float stick_x = joypad.stick_x / 128.0f;

        if (fabsf(stick_x) > 0.1f) {
            float rotation_speed = FPS_ROTATION_SPEED * delta_time;
            cursor_entity->rotation.v[1] += stick_x * rotation_speed;
            cursor_entity->rotation.v[1] = normalize_angle(cursor_entity->rotation.v[1]);

            // Update look direction based on new rotation
            cursor_look_direction.v[0] = sinf(cursor_entity->rotation.v[1]);
            cursor_look_direction.v[2] = -cosf(cursor_entity->rotation.v[1]);
        }

        // Look in same direction cursor is facing
        camera.target.v[0] = cursor_position.v[0] - cursor_look_direction.v[0] * look_distance;
        camera.target.v[1] = cursor_position.v[1] + eye_height;
        camera.target.v[2] = cursor_position.v[2] - cursor_look_direction.v[2] * look_distance;

    } else {
        // Isometric mode: original camera behavior
        float pitch_rad = T3D_DEG_TO_RAD(CAM_ANGLE_PITCH);
        float yaw_rad = T3D_DEG_TO_RAD(cam_yaw);

        float horizontal_dist = CAM_DISTANCE * cosf(pitch_rad);
        float vertical_dist = CAM_DISTANCE * sinf(pitch_rad);

        float follow_speed = CAM_FOLLOW_SPEED * delta_time;
        if (follow_speed > 1.0f) follow_speed = 1.0f;

        camera.target.v[0] += (cursor_position.v[0] - camera.target.v[0]) * follow_speed;
        camera.target.v[2] += (cursor_position.v[2] - camera.target.v[2]) * follow_speed;

        camera.target.v[0] = clampf(camera.target.v[0], -PLAY_AREA_HALF_X, PLAY_AREA_HALF_X);
        camera.target.v[2] = clampf(camera.target.v[2], -PLAY_AREA_HALF_Z, PLAY_AREA_HALF_Z);

        camera.position.v[0] = camera.target.v[0] + horizontal_dist * sinf(yaw_rad);
        camera.position.v[1] = camera.target.v[1] + vertical_dist;
        camera.position.v[2] = camera.target.v[2] + horizontal_dist * cosf(yaw_rad);
    }

    t3d_viewport_set_perspective(viewport, T3D_DEG_TO_RAD(CAM_DEFAULT_FOV),
                                  CAM_ASPECT_RATIO, CAM_NEAR_PLANE, CAM_FAR_PLANE);

    T3DVec3 up = {{0, 1, 0}};
    t3d_viewport_look_at(viewport, &camera.position, &camera.target, &up);
}