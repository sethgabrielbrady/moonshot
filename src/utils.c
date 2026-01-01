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

// static float fps_min = 9999.0f;
// static float fps_max = 0.0f;
// static float fps_current = 0.0f;
// static float fps_avg = 0.0f;
// static float fps_total = 0.0f;
// static int fps_frame_count = 0;

// void reset_fps_stats(void) {
//     fps_min = 9999.0f;
//     fps_max = 0.0f;
//     fps_total = 0.0f;
//     fps_frame_count = 0;
//     fps_avg = 0.0f;
// }

// void update_fps_stats(float delta_time) {
//     fps_current = 1.0f / delta_time;

//     if (fps_current > 1.0f && fps_current < 1000.0f) {
//         if (fps_current < fps_min) fps_min = fps_current;
//         if (fps_current > fps_max) fps_max = fps_current;

//         fps_total += fps_current;
//         fps_frame_count++;
//         fps_avg = fps_total / fps_frame_count;
//     }
// }


// static void update_cursor(float delta_time, float cam_yaw) {
//     joypad_inputs_t joypad = joypad_get_inputs(JOYPAD_PORT_1);
//     joypad_buttons_t held = joypad_get_buttons_held(JOYPAD_PORT_1);

//     // Apply deadzone
//     float stick_x = joypad.stick_x;
//     float stick_y = joypad.stick_y;
//     if (fabsf(stick_x) < 12) stick_x = 0;
//     if (fabsf(stick_y) < 12) stick_y = 0;

//     float yaw_rad = T3D_DEG_TO_RAD(cam_yaw);
//     float cos_yaw = cosf(yaw_rad);
//     float sin_yaw = sinf(yaw_rad);

//     float rotated_x = (stick_x * cos_yaw - stick_y * sin_yaw);
//     float rotated_z = (stick_x * sin_yaw + stick_y * cos_yaw);

//     // Clamp small rotated values to zero
//     if (fabsf(rotated_x) < 5.0f) rotated_x = 0.0f;
//     if (fabsf(rotated_z) < 5.0f) rotated_z = 0.0f;

//     float stick_magnitude = sqrtf(stick_x * stick_x + stick_y * stick_y);

//     if (fps_mode) {
//         float norm_stick_y = stick_y / 128.0f;

//         if (fabsf(norm_stick_y) > 0.1f && cursor_entity) {
//             float thrust_x = -sinf(cursor_entity->rotation.v[1]) * norm_stick_y * 128.0f;
//             float thrust_z = -cosf(cursor_entity->rotation.v[1]) * norm_stick_y * 128.0f;

//             // Clamp small thrust values
//             if (fabsf(thrust_x) < 1.0f) thrust_x = 0.0f;
//             if (fabsf(thrust_z) < 1.0f) thrust_z = 0.0f;

//             cursor_velocity.v[0] += thrust_x * CURSOR_THRUST * delta_time;
//             cursor_velocity.v[2] -= thrust_z * CURSOR_THRUST * delta_time;

//         }

//     } else {
//         if (stick_magnitude > CURSOR_DEADZONE && cursor_entity) {
//             cursor_entity->rotation.v[1] = atan2f(-rotated_x, -rotated_z);

//             cursor_look_direction.v[0] = -rotated_x / stick_magnitude;
//             cursor_look_direction.v[2] = rotated_z / stick_magnitude;
//         }

//         if (stick_magnitude > CURSOR_DEADZONE) {
//             float thrust_x = rotated_x * CURSOR_THRUST * delta_time;
//             float thrust_z = rotated_z * CURSOR_THRUST * delta_time;

//             // Clamp small thrust values
//             if (fabsf(thrust_x) < 0.01f) thrust_x = 0.0f;
//             if (fabsf(thrust_z) < 0.01f) thrust_z = 0.0f;

//             cursor_velocity.v[0] += thrust_x;
//             cursor_velocity.v[2] -= thrust_z;
//         }
//     }

//     // Apply drag
//     float drag = CURSOR_DRAG * delta_time;
//     if (drag > 1.0f) drag = 1.0f;

//     cursor_velocity.v[0] *= (1.0f - drag);
//     cursor_velocity.v[2] *= (1.0f - drag);

//     // Clamp small velocity components
//     if (fabsf(cursor_velocity.v[0]) < 1.0f) cursor_velocity.v[0] = 0.0f;
//     if (fabsf(cursor_velocity.v[2]) < 1.0f) cursor_velocity.v[2] = 0.0f;

//     // Clamp velocity to max speed
//     float speed = sqrtf(cursor_velocity.v[0] * cursor_velocity.v[0] +
//                         cursor_velocity.v[2] * cursor_velocity.v[2]);
//     if (speed > CURSOR_MAX_SPEED) {
//         float scale = CURSOR_MAX_SPEED / speed;
//         cursor_velocity.v[0] *= scale;
//         cursor_velocity.v[2] *= scale;
//     }

//     // Stop completely if very slow
//     if (speed < 2.0f) {
//         cursor_velocity.v[0] = 0.0f;
//         cursor_velocity.v[2] = 0.0f;
//     }

//     // Apply velocity to position
//     cursor_position.v[0] += cursor_velocity.v[0] * delta_time;
//     cursor_position.v[2] += cursor_velocity.v[2] * delta_time;

//     // Round position to reduce jitter
//     cursor_position.v[0] = roundf(cursor_position.v[0] * 10.0f) / 10.0f;
//     cursor_position.v[2] = roundf(cursor_position.v[2] * 10.0f) / 10.0f;

//     if (cursor_entity) {
//         cursor_entity->position = cursor_position;
//         clamp_cursor_position();
//     }
// }
