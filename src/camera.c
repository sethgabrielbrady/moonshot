#include "camera.h"
#include "constants.h"
#include "utils.h"
#include <math.h>


Camera camera = {
    .position = {{0.0f, 0.0f, 0.0f}},
    .target = {{0.0f, 0.0f, 0.0f}},
    .rotation_y = 0.0f
};

extern T3DVec3 cursor_look_direction;

float last_shake_timer = 0.0f;

float screen_shake_timer = 0.0f;
float screen_shake_intensity = 0.0f;
bool camera_zoom_to_station = false;
float camera_zoom_timer = 0.0f;
T3DVec3 camera_zoom_target = {{0, 0, 0}};
static float camera_zoom_duration = 0.0f;


void trigger_screen_shake(float intensity, float duration) {
    screen_shake_intensity = intensity;
    screen_shake_timer = duration;
}

void trigger_camera_zoom(T3DVec3 target, float duration) {
    camera_zoom_to_station = true;
    camera_zoom_timer = duration;
    camera_zoom_duration = duration;
    camera_zoom_target = target;
}

void update_screen_shake(float delta_time) {
    if (screen_shake_timer > 0) {
        screen_shake_timer -= delta_time;
        if (screen_shake_timer <= 0) {
            screen_shake_intensity = 0;
            screen_shake_timer = 0;
            other_shake_enabled = false;
        }
    }
}



// Cache for isometric camera calculations
static float cached_horizontal_dist = 0.0f;
static float cached_vertical_dist = 0.0f;
static float cached_pitch_rad = 0.0f;
static bool cache_initialized = false;

static inline void init_camera_cache(void) {
    if (!cache_initialized) {
        cached_pitch_rad = T3D_DEG_TO_RAD(CAM_ANGLE_PITCH);
        cached_horizontal_dist = CAM_DISTANCE * cosf(cached_pitch_rad);
        cached_vertical_dist = CAM_DISTANCE * sinf(cached_pitch_rad);
        cache_initialized = true;
    }
}

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

// void update_camera(T3DViewport *viewport, float cam_yaw, float delta_time, T3DVec3 cursor_position, bool fps_mode, Entity *cursor_entity) {

// if (fps_mode && cursor_entity) {
//     const float eye_height = 13.0f;
//     const float look_distance = 100.0f;
//     const float camera_offset = 8.0f;

//     // Rotate with joystick X
//     joypad_buttons_t held = joypad_get_buttons_held(JOYPAD_PORT_1);
//     joypad_inputs_t joypad = joypad_get_inputs(JOYPAD_PORT_1);
//     float stick_x = joypad.stick_x / 128.0f;
//     float abs_stick_x = fabsf(stick_x);

//     if (abs_stick_x > 0.1f) {
//         float new_rotation_speed = FPS_ROTATION_SPEED;
//         if (held.r || held.z) {
//             new_rotation_speed *= 2.0f;  // Faster rotation when R or Z held
//         }
//         float rotation_delta = stick_x * new_rotation_speed * delta_time;
//         cursor_entity->rotation.v[1] = normalize_angle(cursor_entity->rotation.v[1] + rotation_delta);

//         // Update look direction - cache sin/cos for reuse
//         float rotation = cursor_entity->rotation.v[1];
//         cursor_look_direction.v[0] = sinf(rotation);
//         cursor_look_direction.v[2] = -cosf(rotation);
//     }

//     // Cache cursor position Y + eye height (used twice)
//     float eye_y = cursor_position.v[1] + eye_height;

//     // Camera position offset behind cursor
//     camera.position.v[0] = cursor_position.v[0] + cursor_look_direction.v[0] * camera_offset;
//     camera.position.v[1] = eye_y;
//     camera.position.v[2] = cursor_position.v[2] + cursor_look_direction.v[2] * camera_offset;

//     // Look in same direction cursor is facing
//     camera.target.v[0] = cursor_position.v[0] - cursor_look_direction.v[0] * look_distance;
//     camera.target.v[1] = eye_y;
//     camera.target.v[2] = cursor_position.v[2] - cursor_look_direction.v[2] * look_distance;


//     } else {
//         // Isometric mode: original camera behavior with cached values
//         init_camera_cache();

//         float yaw_rad = T3D_DEG_TO_RAD(cam_yaw);
//         float sin_yaw = sinf(yaw_rad);
//         float cos_yaw = cosf(yaw_rad);

//         float follow_speed = CAM_FOLLOW_SPEED * delta_time;
//         if (follow_speed > 1.0f) follow_speed = 1.0f;

//         camera.target.v[0] += (cursor_position.v[0] - camera.target.v[0]) * follow_speed;
//         camera.target.v[2] += (cursor_position.v[2] - camera.target.v[2]) * follow_speed;

//         camera.target.v[0] = clampf(camera.target.v[0], -PLAY_AREA_HALF_X, PLAY_AREA_HALF_X);
//         camera.target.v[2] = clampf(camera.target.v[2], -PLAY_AREA_HALF_Z, PLAY_AREA_HALF_Z);

//         camera.position.v[0] = camera.target.v[0] + cached_horizontal_dist * sin_yaw;
//         camera.position.v[1] = camera.target.v[1] + cached_vertical_dist;
//         camera.position.v[2] = camera.target.v[2] + cached_horizontal_dist * cos_yaw;
//     }

//        // Apply screen shake at the end
//     if (screen_shake_timer > 0) {
//         float shake_x = ((rand() % 100) / 100.0f - 0.5f) * screen_shake_intensity;
//         float shake_y = ((rand() % 100) / 100.0f - 0.5f) * screen_shake_intensity;
//         camera.position.v[0] += shake_x;
//         camera.position.v[1] += shake_y;
//     }

//     t3d_viewport_set_perspective(viewport, T3D_DEG_TO_RAD(CAM_DEFAULT_FOV),
//                                   CAM_ASPECT_RATIO, CAM_NEAR_PLANE, CAM_FAR_PLANE);

//     T3DVec3 up = {{0, 1, 0}};
//     t3d_viewport_look_at(viewport, &camera.position, &camera.target, &up);


// }

// void update_camera(T3DViewport *viewport, float cam_yaw, float delta_time, T3DVec3 cursor_position, bool fps_mode, Entity *cursor_entity) {

//     // Handle camera zoom to station
//     if (camera_zoom_to_station) {
//         camera_zoom_timer -= delta_time;

//         if (camera_zoom_timer <= 0) {
//             camera_zoom_to_station = false;
//         } else {
//             // Smoothly move camera target to station
//             float zoom_speed = 3.0f * delta_time;

//             camera.target.v[0] += (camera_zoom_target.v[0] - camera.target.v[0]) * zoom_speed;
//             camera.target.v[1] += (camera_zoom_target.v[1] - camera.target.v[1]) * zoom_speed;
//             camera.target.v[2] += (camera_zoom_target.v[2] - camera.target.v[2]) * zoom_speed;

//             // Position camera looking at station from above
//             float pitch_rad = T3D_DEG_TO_RAD(CAM_ANGLE_PITCH);
//             float yaw_rad = T3D_DEG_TO_RAD(cam_yaw);

//             float zoom_distance = CAM_DISTANCE * 0.6f;  // Closer zoom
//             float horizontal_dist = zoom_distance * cosf(pitch_rad);
//             float vertical_dist = zoom_distance * sinf(pitch_rad);

//             camera.position.v[0] = camera.target.v[0] + horizontal_dist * sinf(yaw_rad);
//             camera.position.v[1] = camera.target.v[1] + vertical_dist;
//             camera.position.v[2] = camera.target.v[2] + horizontal_dist * cosf(yaw_rad);

//             // Apply screen shake
//             if (screen_shake_timer > 0) {
//                 float shake_x = ((rand() % 100) / 100.0f - 0.5f) * screen_shake_intensity;
//                 float shake_y = ((rand() % 100) / 100.0f - 0.5f) * screen_shake_intensity;
//                 camera.position.v[0] += shake_x;
//                 camera.position.v[1] += shake_y;
//             }

//             // Set viewport and return early
//             t3d_viewport_set_perspective(viewport, T3D_DEG_TO_RAD(CAM_DEFAULT_FOV),
//                                           CAM_ASPECT_RATIO, CAM_NEAR_PLANE, CAM_FAR_PLANE);
//             T3DVec3 up = {{0, 1, 0}};
//             t3d_viewport_look_at(viewport, &camera.position, &camera.target, &up);
//             return;
//         }
//     }

//     // Rest of normal camera code...
//     if (fps_mode && cursor_entity) {
//         // ... fps mode code
//     } else {
//         // ... isometric mode code
//     }

//     // Apply screen shake (keep existing code)
//     if (screen_shake_timer > 0) {
//         float shake_x = ((rand() % 100) / 100.0f - 0.5f) * screen_shake_intensity;
//         float shake_y = ((rand() % 100) / 100.0f - 0.5f) * screen_shake_intensity;
//         camera.position.v[0] += shake_x;
//         camera.position.v[1] += shake_y;
//     }

//     t3d_viewport_set_perspective(viewport, T3D_DEG_TO_RAD(CAM_DEFAULT_FOV),
//                                   CAM_ASPECT_RATIO, CAM_NEAR_PLANE, CAM_FAR_PLANE);
//     T3DVec3 up = {{0, 1, 0}};
//     t3d_viewport_look_at(viewport, &camera.position, &camera.target, &up);
// }

void update_camera(T3DViewport *viewport, float cam_yaw, float delta_time, T3DVec3 cursor_position, bool fps_mode, Entity *cursor_entity) {

    // Handle camera zoom to station
    if (camera_zoom_to_station) {
        camera_zoom_timer -= delta_time;

        if (camera_zoom_timer <= 0) {
            camera_zoom_to_station = false;


            if (screen_shake_timer <= 0.010) {
                game_over = true;
            }

        } else {
            // Smoothly move camera target to station
            float zoom_speed = 3.0f * delta_time;

            camera.target.v[0] += (camera_zoom_target.v[0] - camera.target.v[0]) * zoom_speed;
            camera.target.v[1] += (camera_zoom_target.v[1] - camera.target.v[1]) * zoom_speed;
            camera.target.v[2] += (camera_zoom_target.v[2] - camera.target.v[2]) * zoom_speed;

            // Position camera looking at station from above
            float pitch_rad = T3D_DEG_TO_RAD(CAM_ANGLE_PITCH);
            float yaw_rad = T3D_DEG_TO_RAD(cam_yaw);

            float zoom_distance = CAM_DISTANCE * 0.6f;  // Closer zoom
            float horizontal_dist = zoom_distance * cosf(pitch_rad);
            float vertical_dist = zoom_distance * sinf(pitch_rad);

            camera.position.v[0] = camera.target.v[0] + horizontal_dist * sinf(yaw_rad);
            camera.position.v[1] = camera.target.v[1] + vertical_dist;
            camera.position.v[2] = camera.target.v[2] + horizontal_dist * cosf(yaw_rad);

            // Apply screen shake
            if (screen_shake_timer > 0) {
                float shake_x = ((rand() % 100) / 100.0f - 0.5f) * screen_shake_intensity;
                float shake_y = ((rand() % 100) / 100.0f - 0.5f) * screen_shake_intensity;
                camera.position.v[0] += shake_x;
                camera.position.v[1] += shake_y;
            }


            // Set viewport and return early
            t3d_viewport_set_perspective(viewport, T3D_DEG_TO_RAD(CAM_DEFAULT_FOV),
                                          CAM_ASPECT_RATIO, CAM_NEAR_PLANE, CAM_FAR_PLANE);
            T3DVec3 up = {{0, 1, 0}};
            t3d_viewport_look_at(viewport, &camera.position, &camera.target, &up);
            return;
        }
    }

    // Rest of normal camera code - UNCOMMENT AND ADD THIS:
    if (fps_mode && cursor_entity) {
        const float eye_height = 13.0f;
        const float look_distance = 100.0f;
        const float camera_offset = 8.0f;

        // Rotate with joystick X
        joypad_buttons_t held = joypad_get_buttons_held(JOYPAD_PORT_1);
        joypad_inputs_t joypad = joypad_get_inputs(JOYPAD_PORT_1);
        float stick_x = joypad.stick_x / 128.0f;
        float abs_stick_x = fabsf(stick_x);

        if (abs_stick_x > 0.1f) {
            float new_rotation_speed = FPS_ROTATION_SPEED;
            if (held.r || held.z) {
                new_rotation_speed *= 2.0f;
            }
            float rotation_delta = stick_x * new_rotation_speed * delta_time;
            cursor_entity->rotation.v[1] = normalize_angle(cursor_entity->rotation.v[1] + rotation_delta);

            float rotation = cursor_entity->rotation.v[1];
            cursor_look_direction.v[0] = sinf(rotation);
            cursor_look_direction.v[2] = -cosf(rotation);
        }

        float eye_y = cursor_position.v[1] + eye_height;

        camera.position.v[0] = cursor_position.v[0] + cursor_look_direction.v[0] * camera_offset;
        camera.position.v[1] = eye_y;
        camera.position.v[2] = cursor_position.v[2] + cursor_look_direction.v[2] * camera_offset;

        camera.target.v[0] = cursor_position.v[0] - cursor_look_direction.v[0] * look_distance;
        camera.target.v[1] = eye_y;
        camera.target.v[2] = cursor_position.v[2] - cursor_look_direction.v[2] * look_distance;

    } else {
        // Isometric mode
        init_camera_cache();

        float yaw_rad = T3D_DEG_TO_RAD(cam_yaw);
        float sin_yaw = sinf(yaw_rad);
        float cos_yaw = cosf(yaw_rad);

        float follow_speed = CAM_FOLLOW_SPEED * delta_time;
        if (follow_speed > 1.0f) follow_speed = 1.0f;

        camera.target.v[0] += (cursor_position.v[0] - camera.target.v[0]) * follow_speed;
        camera.target.v[2] += (cursor_position.v[2] - camera.target.v[2]) * follow_speed;

        camera.target.v[0] = clampf(camera.target.v[0], -PLAY_AREA_HALF_X, PLAY_AREA_HALF_X);
        camera.target.v[2] = clampf(camera.target.v[2], -PLAY_AREA_HALF_Z, PLAY_AREA_HALF_Z);

        camera.position.v[0] = camera.target.v[0] + cached_horizontal_dist * sin_yaw;
        camera.position.v[1] = camera.target.v[1] + cached_vertical_dist;
        camera.position.v[2] = camera.target.v[2] + cached_horizontal_dist * cos_yaw;
    }


    if (other_shake_enabled) {
        float shake_x = ((rand() % 100) / 100.0f - 0.5f) * screen_shake_intensity;
        float shake_y = ((rand() % 100) / 100.0f - 0.5f) * screen_shake_intensity;
        camera.position.v[0] += shake_x;
        camera.position.v[1] += shake_y;
    }

    t3d_viewport_set_perspective(viewport, T3D_DEG_TO_RAD(CAM_DEFAULT_FOV),
                                  CAM_ASPECT_RATIO, CAM_NEAR_PLANE, CAM_FAR_PLANE);
    T3DVec3 up = {{0, 1, 0}};
    t3d_viewport_look_at(viewport, &camera.position, &camera.target, &up);
}