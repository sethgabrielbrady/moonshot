#include "input.h"
#include "constants.h"
#include "game_state.h"
#include "collision.h"
#include "camera.h"
#include "audio.h"
#include "ui.h"
#include "utils.h"
#include <math.h>

// =============================================================================
// Global Input State
// =============================================================================

InputState input = {
    .stick_x = 0.0f,
    .stick_y = 0.0f,
    .stick_magnitude = 0.0f,
    .stick_magnitude_sq = 0.0f,
    .pressed = {0},
    .held = {0},
    .released = {0}
};

// =============================================================================
// Internal State
// =============================================================================

static int menu_input_delay = 0;

// =============================================================================
// Input Update (call once per frame)
// =============================================================================

void update_input(void) {
    joypad_inputs_t joypad = joypad_get_inputs(JOYPAD_PORT_1);

    input.stick_x = joypad.stick_x;
    input.stick_y = joypad.stick_y;
    input.stick_magnitude_sq = input.stick_x * input.stick_x + input.stick_y * input.stick_y;

    // input.stick_x = joypad.stick_x;
    // input.stick_y = joypad.stick_y;
    // input.stick_magnitude_sq = input.stick_x * input.stick_x + input.stick_y * input.stick_y;

    // Use fast inverse sqrt: magnitude = magnitude_sq * (1/sqrt(magnitude_sq))
    if (input.stick_magnitude_sq > 0.0f) {
        input.stick_magnitude = input.stick_magnitude_sq * fast_inv_sqrt(input.stick_magnitude_sq);
    } else {
        input.stick_magnitude = 0.0f;
    }

    input.pressed = joypad_get_buttons_pressed(JOYPAD_PORT_1);
    input.held = joypad_get_buttons_held(JOYPAD_PORT_1);
    input.released = joypad_get_buttons_released(JOYPAD_PORT_1);
}

// =============================================================================
// Menu Input Processing
// =============================================================================

void process_menu_input(void) {

    // Decrease delay timer
    if (menu_input_delay > 0) menu_input_delay--;

    // Navigate up
    if (menu_input_delay == 0 &&
        (input.pressed.d_up || input.pressed.c_up || input.stick_y > 50)) {
        game.menu_selection--;
        if (game.menu_selection < 0) game.menu_selection = MENU_OPTIONS_COUNT - 1;
        menu_input_delay = 10;
    }

    // Navigate down
    if (menu_input_delay == 0 &&
        (input.pressed.d_down || input.pressed.c_down || input.stick_y < -50)) {
        game.menu_selection++;
        if (game.menu_selection >= MENU_OPTIONS_COUNT) game.menu_selection = 0;
        menu_input_delay = 10;
    }

    // Select with A
    if (input.pressed.a) {
        switch (game.menu_selection) {
            case MENU_OPTION_RESUME:
                game.state = STATE_PLAYING;
                game.game_over = false;
                if (game.game_over_pause) {
                    // Reset game state for restart
                    game.game_over_pause = false;
                    game.reset = true;  // <-- Only in input.c

                    // Call reset function (defined elsewhere)
                }
                set_bgm_volume(0.5f);
                break;

            // case MENU_OPTION_CAMERA:
            //     game.fps_mode = !game.fps_mode;
            //     break;

            case MENU_OPTION_HIRES:
                game.hi_res_mode = !game.hi_res_mode;
                display_close();
                if (game.hi_res_mode) {
                    display_init(RESOLUTION_640x240, DEPTH_16_BPP, 3, GAMMA_NONE, FILTERS_RESAMPLE_ANTIALIAS);
                } else {
                    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 3, GAMMA_NONE, FILTERS_RESAMPLE_ANTIALIAS);
                }
                break;

            case MENU_OPTION_AUDIO:
                game.bgm_track = (game.bgm_track + 1) % 4;
                stop_bgm();
                if (game.bgm_track == 1) {
                    play_bgm("rom:/nebrun.wav64");
                } else if (game.bgm_track == 2) {
                    play_bgm("rom:/orbodd.wav64");
                } else if (game.bgm_track == 3) {
                    play_bgm("rom:/lunram.wav64");
                }
                break;

            case MENU_OPTION_30HZ:
                game.fps_limit = (game.fps_limit + 1) % 3;
                if (game.fps_limit == 0) {
                    display_set_fps_limit(game.is_pal_system ? 25 : 30);
                } else if (game.fps_limit == 1) {
                    display_set_fps_limit(game.is_pal_system ? 50 : 60);
                } else {
                    display_set_fps_limit(0);  // Uncapped
                }
                break;

            case MENU_OPTION_BG:
                game.render_background_enabled = !game.render_background_enabled;
                break;
        }
    }
}

// =============================================================================
// System Input (debug, pause toggle - always processed)
// =============================================================================

void process_system_input(T3DViewport *viewport) {
    // Toggle pause with START
    if (input.pressed.start) {
        if (game.state == STATE_PLAYING) {
            game.state = STATE_PAUSED;
            game.menu_selection = 0;
            menu_input_delay = 0;
            set_bgm_volume(0.25f);
        } else if (game.state == STATE_PAUSED) {
            game.state = STATE_PLAYING;
            set_bgm_volume(0.5f);
        }
        return;
    }

    // Handle paused state
    if (game.state == STATE_PAUSED || game.game_over) {
        process_menu_input();

        // Recreate viewport if resolution changed
        if (input.pressed.a && game.menu_selection == MENU_OPTION_HIRES) {
            *viewport = t3d_viewport_create();
        }
        return;
    }

    // Debug toggles (only when not paused)
    if (input.pressed.d_up) game.render_debug = !game.render_debug;
    if (input.pressed.d_left) game.show_fps = !game.show_fps;
    if (input.pressed.l) reset_fps_stats();

    // // Camera mode toggle
    if (input.pressed.d_down) {
        game.fps_mode = !game.fps_mode;
    }
}

// =============================================================================
// Game Input (movement, actions - only when not paused)
// =============================================================================

void process_game_input(float delta_time) {


    check_deflect_input();

    // Camera rotation (isometric mode only)Pspaw
    if (!game.fps_mode) {
        float rotation_speed = CAM_ROTATION_SPEED * delta_time;
        if (input.held.r) game.cam_yaw -= rotation_speed;
        if (input.held.z) game.cam_yaw += rotation_speed;
    }

    // Drone heal mode
    if (input.pressed.c_up) {
        game.drone_heal = true;

        game.drone_target_position.v[0] = game.cursor_position.v[0];
        game.drone_target_position.v[1] = game.cursor_position.v[1];
        game.drone_target_position.v[2] = game.cursor_position.v[2];
        game.move_drone = true;
        game.drone_moving_to_resource = false;
        game.drone_moving_to_station = false;
        game.tile_following_resource = -1;
        play_sfx(2);
    }

    // Send drone to tile position
    if (input.pressed.c_left) {
        float offset_distance = 30.0f;
        game.drone_target_position.v[0] = game.cursor_position.v[0] - cursor_look_direction.v[0] * offset_distance;
        game.drone_target_position.v[1] = game.cursor_position.v[1];
        game.drone_target_position.v[2] = game.cursor_position.v[2] - cursor_look_direction.v[2] * offset_distance;
        game.move_drone = true;
        game.drone_heal = false;
        game.drone_moving_to_resource = true;
        game.drone_moving_to_station = false;
        game.tile_following_resource = -1;
        play_sfx(2);
    }

    // Send drone to station
    if (input.pressed.c_down) {
        game.drone_target_rotation = 0.0f;
        game.drone_target_position.v[0] = 0.0f;
        game.drone_target_position.v[2] = 0.0f;
        game.move_drone = true;
        game.drone_moving_to_station = true;
        game.drone_moving_to_resource = false;
        game.drone_heal = false;
    }
}

// =============================================================================
// Cursor Movement Update
// =============================================================================

void update_cursor_movement(float delta_time, Entity *cursor_entity) {
    float yaw_rad = T3D_DEG_TO_RAD(game.cam_yaw);
    float cos_yaw = cosf(yaw_rad);
    float sin_yaw = sinf(yaw_rad);

    float rotated_x = (input.stick_x * cos_yaw - input.stick_y * sin_yaw);
    float rotated_z = (input.stick_x * sin_yaw + input.stick_y * cos_yaw);

    float deadzone_sq = CURSOR_DEADZONE * CURSOR_DEADZONE;

    //if stick magnitude greated than deadzone, decrease game.ship_fuel
    if (input.stick_magnitude_sq > deadzone_sq) {
        game.ship_acceleration = true;
    } else {
        game.ship_acceleration = false;
    }

    if (game.fps_mode) {
        // FPS mode: thrust forward/backward only
        float stick_y = input.stick_y / 128.0f;

        if (fabsf(stick_y) > 0.1f && cursor_entity) {
            float thrust_x = -sinf(cursor_entity->rotation.v[1]) * stick_y * 128.0f;
            float thrust_z = -cosf(cursor_entity->rotation.v[1]) * stick_y * 128.0f;
            game.cursor_velocity.v[0] += thrust_x * CURSOR_THRUST * delta_time;
            game.cursor_velocity.v[2] -= thrust_z * CURSOR_THRUST * delta_time;
        }

    } else {
        // Isometric mode: rotate cursor to face stick direction
        if (input.stick_magnitude_sq > deadzone_sq && cursor_entity) {
            cursor_entity->rotation.v[1] = atan2f(-rotated_x, -rotated_z);

            cursor_look_direction.v[0] = -rotated_x / input.stick_magnitude;
            cursor_look_direction.v[2] = rotated_z / input.stick_magnitude;
        }

        // Thrust in joystick direction
        if (input.stick_magnitude_sq > deadzone_sq) {
            game.cursor_velocity.v[0] += rotated_x * CURSOR_THRUST * delta_time;
            game.cursor_velocity.v[2] -= rotated_z * CURSOR_THRUST * delta_time;
        }
    }

    // Apply drag
    float drag = CURSOR_DRAG * delta_time;
    if (drag > 1.0f) drag = 1.0f;

    game.cursor_velocity.v[0] *= (1.0f - drag);
    game.cursor_velocity.v[2] *= (1.0f - drag);

    // Clamp velocity to max speed
    float speed_sq = game.cursor_velocity.v[0] * game.cursor_velocity.v[0] +
                     game.cursor_velocity.v[2] * game.cursor_velocity.v[2];
    float max_speed_sq = CURSOR_MAX_SPEED * CURSOR_MAX_SPEED;

    if (speed_sq > max_speed_sq) {
        // Use fast inverse sqrt: scale = max_speed / speed = max_speed * (1/sqrt(speed_sq))
        float scale = CURSOR_MAX_SPEED * fast_inv_sqrt(speed_sq);
        game.cursor_velocity.v[0] *= scale;
        game.cursor_velocity.v[2] *= scale;
    }

    // Stop completely if very slow
    if (speed_sq < 0.01f) {
        game.cursor_velocity.v[0] = 0.0f;
        game.cursor_velocity.v[2] = 0.0f;
    }

    // Apply velocity to position
    game.cursor_position.v[0] += game.cursor_velocity.v[0] * delta_time;
    game.cursor_position.v[2] += game.cursor_velocity.v[2] * delta_time;

    // Round position to reduce jitter
    game.cursor_position.v[0] = roundf(game.cursor_position.v[0] * 10.0f) / 10.0f;
    game.cursor_position.v[2] = roundf(game.cursor_position.v[2] * 10.0f) / 10.0f;

    // Clamp to play area
    game.cursor_position.v[0] = clampf(game.cursor_position.v[0], -PLAY_AREA_HALF_X, PLAY_AREA_HALF_X);
    game.cursor_position.v[2] = clampf(game.cursor_position.v[2], -PLAY_AREA_HALF_Z, PLAY_AREA_HALF_Z);

    // Update cursor entity position
    if (cursor_entity) {
        cursor_entity->position = game.cursor_position;
    }
}