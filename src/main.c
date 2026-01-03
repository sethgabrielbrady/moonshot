#include <libdragon.h>
#include <t3d/t3d.h>
#include <t3d/t3ddebug.h>
#include <rdpq.h>
#include <stdlib.h>
#include <stdint.h>

#include "types.h"
#include "constants.h"
#include "utils.h"
#include "entity.h"
#include "camera.h"
#include "asteroid.h"
#include "audio.h"
#include "debug.h"
#include "particles.h"





// =============================================================================
// Global State
// =============================================================================

static Entity entities[ENTITY_COUNT];
static Entity asteroids[ASTEROID_COUNT];
static Entity resources[RESOURCE_COUNT];
static Entity *cursor_entity = NULL;

static T3DVec3 cursor_position = {{200.0f, CURSOR_HEIGHT, 100.0f}};
static T3DVec3 cursor_velocity = {{0.0f, 0.0f, 0.0f}};

static bool move_drone = false;
static float target_rotation = 0.0f;
static T3DVec3 target_position;

static bool render_debug = false;
static bool resource_intersection = false;
static bool b_press = false;
static bool fps_mode = false;

static int resource_val = 0;
static int drone_resource_val = 0;
static int cursor_resource_val = 0;
static int highlighted_resource = -1;

static float cursor_mining_accumulated = 0.0f;
static float drone_mining_accumulated = 0.0f;

static float last_thrust_x = 0.0f;
static float last_thrust_z = 0.0f;

T3DVec3 cursor_look_direction = {{0.0f, 0.0f, -1.0f}};
static ColorFlash color_flashes[MAX_COLOR_FLASHES];

static T3DBvh bvh;
static bool use_culling = true;
static bool drone_heal = false;
static bool drone_collecting_resource = false;
static bool drone_moving_to_resource = false;
static bool drone_moving_to_station = false;


static float fps_min = 9999.0f;
static float fps_max = 0.0f;
static float fps_current = 0.0f;
static float fps_avg = 0.0f;
static float fps_total = 0.0f;
static int fps_frame_count = 0;

void reset_fps_stats(void) {
    fps_min = 9999.0f;
    fps_max = 0.0f;
    fps_total = 0.0f;
    fps_frame_count = 0;
    fps_avg = 0.0f;
}

void update_fps_stats(float delta_time) {
    fps_current = 1.0f / delta_time;

    if (fps_current > 1.0f && fps_current < 1000.0f) {
        if (fps_current < fps_min) fps_min = fps_current;
        if (fps_current > fps_max) fps_max = fps_current;

        fps_total += fps_current;
        fps_frame_count++;
        fps_avg = fps_total / fps_frame_count;
    }
}

static void draw_fps_display(void) {
    int x = SCREEN_WIDTH - 120;
    int y = 10;
    int line_height = 10;

    rdpq_sync_pipe();

    rdpq_text_printf(NULL, FONT_BUILTIN_DEBUG_MONO, x, y,
                     "fps: %.0f avg: %.0f", fps_current, fps_avg);
    y += line_height;
    rdpq_text_printf(NULL, FONT_BUILTIN_DEBUG_MONO, x, y,
                     "min: %.0f max: %.0f", fps_min, fps_max);


    y += line_height;
    rdpq_text_printf(NULL, FONT_BUILTIN_DEBUG_MONO, x, y,
                     "Particles: %d", debug_particle_count);
}

void draw_pause_menu(void) {
    int padding_x = SCREEN_WIDTH * 0.2f;
    int padding_y = SCREEN_HEIGHT * 0.15f;

    int x1 = padding_x;
    int y1 = padding_y;
    int x2 = SCREEN_WIDTH - padding_x;
    int y2 = SCREEN_HEIGHT - padding_y;

    rdpq_sync_pipe();
    rdpq_mode_combiner(RDPQ_COMBINER_FLAT);

    // Draw teal box
    rdpq_set_prim_color(RGBA32(0, 128, 128, 255));
    rdpq_fill_rectangle(x1, y1, x2, y2);

    // Draw border
    rdpq_set_prim_color(RGBA32(0, 200, 200, 255));
    rdpq_fill_rectangle(x1, y1, x2, y1 + 2);
    rdpq_fill_rectangle(x1, y2 - 2, x2, y2);
    rdpq_fill_rectangle(x1, y1, x1 + 2, y2);
    rdpq_fill_rectangle(x2 - 2, y1, x2, y2);

    int menu_x = x1 + 15;
    int menu_y = y1 + 40;
    int line_height = 18;

    // Draw highlight box behind selected option
    rdpq_set_prim_color(RGBA32(0, 180, 180, 255));
    rdpq_fill_rectangle(menu_x - 5, menu_y + (menu_selection * line_height) - 10,
                        x2 - 15, menu_y + (menu_selection * line_height) + 5);

    rdpq_sync_pipe();

    // Title
    rdpq_text_printf(NULL, FONT_CUSTOM, 120, y1 + 15, "AsteRisk");

    // Menu options
    rdpq_text_printf(NULL, FONT_CUSTOM, menu_x, menu_y, "Resume");
    rdpq_text_printf(NULL, FONT_CUSTOM, menu_x, menu_y + line_height,
                     "Camera: %s", fps_mode ? "FPS" : "ISO");
    rdpq_text_printf(NULL, FONT_CUSTOM, menu_x, menu_y + line_height * 2,
                     "Debug: %s", render_debug ? "ON" : "OFF");
    rdpq_text_printf(NULL, FONT_CUSTOM, menu_x, menu_y + line_height * 3,
                     "Music: %s", bgm_playing ? "ON" : "OFF");
    rdpq_text_printf(NULL, FONT_CUSTOM, menu_x, menu_y + line_height * 4,
                     "FPS Limit: %s", limit30hz ? "30" : "60");

    // Controls hint
    rdpq_text_printf(NULL, FONT_CUSTOM, menu_x, y2 - 20,
                     "Up/Down: Select  A: Toggle");
}

static bool is_entity_in_frustum(Entity *entity, T3DVec3 cam_position, T3DVec3 cam_target, float fov_degrees) {
    // Direction from camera to entity
    float dx = entity->position.v[0] - cam_position.v[0];
    float dy = entity->position.v[1] - cam_position.v[1];
    float dz = entity->position.v[2] - cam_position.v[2];

    // Squared distance to entity
    float distance_sq = dx * dx + dy * dy + dz * dz;
    if (distance_sq < 0.000001f) return true;  // Entity at camera position

    // Normalize direction to entity using fast inverse sqrt
    float inv_dist = fast_inv_sqrt(distance_sq);
    dx *= inv_dist;
    dy *= inv_dist;
    dz *= inv_dist;

    // Camera forward direction
    float fx = cam_target.v[0] - cam_position.v[0];
    float fy = cam_target.v[1] - cam_position.v[1];
    float fz = cam_target.v[2] - cam_position.v[2];

    float forward_len_sq = fx * fx + fy * fy + fz * fz;
    if (forward_len_sq < 0.000001f) return true;

    float inv_forward = fast_inv_sqrt(forward_len_sq);
    fx *= inv_forward;
    fy *= inv_forward;
    fz *= inv_forward;

    // Dot product (cosine of angle between camera forward and direction to entity)
    float dot = dx * fx + dy * fy + dz * fz;

    // Convert FOV to cosine threshold (with some padding for object radius)
    float half_fov_rad = T3D_DEG_TO_RAD(fov_degrees * 0.5f);
    float cos_threshold = cosf(half_fov_rad * 1.2f);  // 1.2 = padding

    // Behind camera or outside FOV
    if (dot < cos_threshold) {
        return false;
    }

    // Distance culling - compute actual distance only when needed
    float distance = 1.0f / inv_dist;
    float radius = entity->scale * 50.0f;
    if (distance - radius > CAM_FAR_PLANE) {
        return false;
    }

    return true;
}

static int culled_count = 0;

static void draw_entities_culled(Entity *entity_array, int count, bool *skip_culling) {
    for (int i = 0; i < count; i++) {
        if (skip_culling != NULL && skip_culling[i]) {
            // Always draw, don't cull
            draw_entity(&entity_array[i]);
        } else if (is_entity_in_frustum(&entity_array[i], camera.position, camera.target, CAM_DEFAULT_FOV)) {
            draw_entity(&entity_array[i]);
        } else {
            culled_count++;
        }
    }
}


// =============================================================================
// Initialization
// =============================================================================

static void init_subsystems(void) {
    debug_init_isviewer();
    debug_init_usblog();
    asset_init_compression(2);
    dfs_init(DFS_DEFAULT_LOCATION);
    // display_init(hi_res, DEPTH_16_BPP, 3, GAMMA_NONE, FILTERS_RESAMPLE);
    // display_init(RESOLUTION_320x240, DEPTH_16_BPP, 3, GAMMA_NONE, FILTERS_DISABLED);
    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 3, GAMMA_CORRECT, FILTERS_RESAMPLE_ANTIALIAS);


    // display_set_fps_limit(60);
    display_set_fps_limit(30);

    rdpq_init();

    joypad_init();
    t3d_init((T3DInitParams){});
    init_particles();  // This now calls tpx_init internally


    rdpq_text_register_font(
        FONT_BUILTIN_DEBUG_MONO,
        rdpq_font_load_builtin(FONT_BUILTIN_DEBUG_MONO)
    );

    custom_font = rdpq_font_load("rom:/Nebula.font64"); // fav -- nice slant but a little large- though not a bad thing favor
    // custom_font = rdpq_font_load("rom:/Nulshock.font64"); // decent -- nice slant but a little large- though not a bad thing favor
    //custom_font = rdpq_font_load("rom:/RobotRoc.font64"); // decent -- nice slant but a little large- though not a bad thing favor
    // custom_font = rdpq_font_load("rom:/Induction.font64"); // big but might be good for title
    // custom_font = rdpq_font_load("rom:/H19A-Luna.font64"); // good for symbols

    rdpq_text_register_font(FONT_CUSTOM, custom_font);
    // rdpq_text_register_font(FONT_CUSTOM_TWO, custom_font);

   rdpq_font_style(custom_font, 0, &(rdpq_fontstyle_t){
        .color = RGBA32(255, 255, 255, 255),
        // .outline_color = RGBA32(0, 0, 0, 0),
	});

    // Initialize audio
    audio_init(32000, 4);  // 32kHz sample rate, 4 buffers
    mixer_init(8);        // 8 channels
    set_bgm_volume(0.5f);





    // Seed the random number generator - THIS IS THE KEY LINE
    srand(get_ticks());
}

// =============================================================================
// Cursor
// =============================================================================

static inline void clamp_cursor_position(void) {
    cursor_position.v[0] = clampf(cursor_position.v[0], -PLAY_AREA_HALF_X, PLAY_AREA_HALF_X);
    cursor_position.v[2] = clampf(cursor_position.v[2], -PLAY_AREA_HALF_Z, PLAY_AREA_HALF_Z);
}

static void update_cursor(float delta_time, float cam_yaw) {
    joypad_inputs_t joypad = joypad_get_inputs(JOYPAD_PORT_1);
    joypad_buttons_t held = joypad_get_buttons_held(JOYPAD_PORT_1);

    float yaw_rad = T3D_DEG_TO_RAD(cam_yaw);
    float cos_yaw = cosf(yaw_rad);
    float sin_yaw = sinf(yaw_rad);

    float rotated_x = (joypad.stick_x * cos_yaw - joypad.stick_y * sin_yaw);
    float rotated_z = (joypad.stick_x * sin_yaw + joypad.stick_y * cos_yaw);

    float stick_magnitude_sq = joypad.stick_x * joypad.stick_x +
                               joypad.stick_y * joypad.stick_y;
    float deadzone_sq = CURSOR_DEADZONE * CURSOR_DEADZONE;

    if (fps_mode) {
        // FPS mode: thrust forward/backward only
        float stick_y = joypad.stick_y / 128.0f;

        if (fabsf(stick_y) > 0.1f && cursor_entity) {
            float thrust_x = -sinf(cursor_entity->rotation.v[1]) * stick_y * 128.0f;
            float thrust_z = -cosf(cursor_entity->rotation.v[1]) * stick_y * 128.0f;
            cursor_velocity.v[0] += thrust_x * CURSOR_THRUST * delta_time;
            cursor_velocity.v[2] -= thrust_z * CURSOR_THRUST * delta_time;
        }

    } else {

        // Isometric mode: rotate cursor to face stick direction
        if (stick_magnitude_sq > deadzone_sq && cursor_entity) {
            float stick_magnitude = sqrtf(stick_magnitude_sq);
            cursor_entity->rotation.v[1] = atan2f(-rotated_x, -rotated_z);

            cursor_look_direction.v[0] = -rotated_x / stick_magnitude;
            cursor_look_direction.v[2] = rotated_z / stick_magnitude;
            // spawn_exhaust(cursor_position, cursor_look_direction);

        }

        // Thrust in joystick direction
        if (stick_magnitude_sq > deadzone_sq) {
            cursor_velocity.v[0] += rotated_x * CURSOR_THRUST * delta_time;
            cursor_velocity.v[2] -= rotated_z * CURSOR_THRUST * delta_time;
        }

    }

    // Apply drag
    float drag = CURSOR_DRAG * delta_time;
    if (drag > 1.0f) drag = 1.0f;

    cursor_velocity.v[0] *= (1.0f - drag);
    cursor_velocity.v[2] *= (1.0f - drag);

    // Clamp velocity to max speed using squared comparison
    float speed_sq = cursor_velocity.v[0] * cursor_velocity.v[0] +
                     cursor_velocity.v[2] * cursor_velocity.v[2];
    float max_speed_sq = CURSOR_MAX_SPEED * CURSOR_MAX_SPEED;

    if (speed_sq > max_speed_sq) {
        float speed = sqrtf(speed_sq);
        float scale = CURSOR_MAX_SPEED / speed;
        cursor_velocity.v[0] *= scale;
        cursor_velocity.v[2] *= scale;
    }

    // Stop completely if very slow (0.1f * 0.1f = 0.01f)
    if (speed_sq < 0.01f) {
        cursor_velocity.v[0] = 0.0f;
        cursor_velocity.v[2] = 0.0f;
    }





    // Apply velocity to position
    cursor_position.v[0] += cursor_velocity.v[0] * delta_time;
    cursor_position.v[2] += cursor_velocity.v[2] * delta_time;

    // Round position to reduce jitter
    cursor_position.v[0] = roundf(cursor_position.v[0] * 10.0f) / 10.0f;
    cursor_position.v[2] = roundf(cursor_position.v[2] * 10.0f) / 10.0f;
    if (cursor_entity) {
        cursor_entity->position = cursor_position;
        clamp_cursor_position();
    }
}




static void start_entity_color_flash(Entity *entity, color_t flash_color, float duration_seconds) {
    for (int i = 0; i < MAX_COLOR_FLASHES; i++) {
        if (color_flashes[i].active && color_flashes[i].entity == entity) {
            color_flashes[i].timer = 0.0f;
            color_flashes[i].duration = duration_seconds;
            color_flashes[i].flash_color = flash_color;
            return;
        }
    }

    for (int i = 0; i < MAX_COLOR_FLASHES; i++) {
        if (!color_flashes[i].active) {
            color_flashes[i].entity = entity;
            color_flashes[i].original_color = entity->color;
            color_flashes[i].flash_color = flash_color;
            color_flashes[i].timer = 0.0f;
            color_flashes[i].duration = duration_seconds;
            color_flashes[i].active = true;
            entity->color = flash_color;
            return;
        }
    }
}

static void update_color_flashes(float delta_time) {
    for (int i = 0; i < MAX_COLOR_FLASHES; i++) {
        if (!color_flashes[i].active) continue;

        color_flashes[i].timer += delta_time;

        if (color_flashes[i].timer >= color_flashes[i].duration) {
            color_flashes[i].entity->color = color_flashes[i].original_color;
            color_flashes[i].active = false;
            color_flashes[i].entity = NULL;
        } else {
            int blink_state = (int)(color_flashes[i].timer * 10) % 2;
            if (blink_state == 0) {
                color_flashes[i].entity->color = color_flashes[i].flash_color;
            } else {
                color_flashes[i].entity->color = color_flashes[i].original_color;
            }
        }
    }
}

static float calculate_asteroid_damage(Entity *asteroid) {
    // Mass approximation (volume scales with cube of size)
    float mass = asteroid->scale * asteroid->scale * asteroid->scale;

    // Kinetic energy = 0.5 * mass * velocity^2
    float kinetic_energy = 0.25f * mass * asteroid->speed * asteroid->speed;

    return kinetic_energy * DAMAGE_MULTIPLIER;
}

static float station_iframe_timer = 0.0f;
static const float STATION_IFRAME_DURATION = 1.0f;  // 1 second invincibility

static void check_station_asteroid_collisions(Entity *station, Entity *asteroids, int count, float delta_time) {
    // Update iframe timer
    if (station_iframe_timer > 0.0f) {
        station_iframe_timer -= delta_time;
    }

    for (int i = 0; i < count; i++) {
        if (check_entity_intersection(station, &asteroids[i])) {
            // Only apply damage if not in iframe period
            if (station_iframe_timer <= 0.0f) {
                float damage = calculate_asteroid_damage(&asteroids[i]);
                if (damage >= MAX_DAMAGE) {
                    damage = MAX_DAMAGE;
                }
                spawn_explosion(asteroids[i].position);
                station->value -= damage;
                station_last_damage = (int)damage;  // Store damage
                // Clamp to zero
                if (station->value < 0) {
                    station->value = 0;
                }
                // Visual feedback


                // start_entity_color_flash(station, RGBA32(255, 0, 0, 255), 0.5f);

                // Start iframe period
                station_iframe_timer = STATION_IFRAME_DURATION;
            }

            // Always reset asteroid to prevent multiple collisions
            reset_entity(&asteroids[i], ASTEROID);
        }
    }
}

static void check_cursor_asteroid_collisions(Entity *cursor, Entity *asteroids, int count) {
    for (int i = 0; i < count; i++) {
        if (check_entity_intersection(cursor, &asteroids[i])) {
            float damage = calculate_asteroid_damage(&asteroids[i]);
            if (damage >= MAX_DAMAGE) {
                damage = MAX_DAMAGE* .75f;
            }
            spawn_explosion(asteroids[i].position);
            cursor->value -= damage;
            cursor_last_damage = (int)damage;  // Store damage

            if (cursor->value < 0) {
                cursor->value = 0;
            }


            // start_entity_color_flash(cursor, RGBA32(255, 0, 0, 255), 0.5f);

            // Add asteroid velocity to cursor velocity (knockback)
            cursor_velocity.v[0] += asteroids[i].velocity.v[0] * KNOCKBACK_STRENGTH;
            cursor_velocity.v[2] += asteroids[i].velocity.v[2] * KNOCKBACK_STRENGTH;


            reset_entity(&asteroids[i], ASTEROID);
        }
    }
}



// =============================================================================
// Drone Movement
// =============================================================================

static void move_drone_elsewhere(Entity *drone, float delta_time, bool base_return) {

    if (base_return) {
        target_position.v[0] = 0.0f;
        target_position.v[2] = 0.0f;
    }


    float dx = target_position.v[0] - drone->position.v[0];
    float dz = target_position.v[2] - drone->position.v[2];
    float distance_sq = dx * dx + dz * dz;

    if (distance_sq < DRONE_ARRIVE_THRESHOLD * DRONE_ARRIVE_THRESHOLD) {
        move_drone = false;
        drone_moving_to_resource = move_drone;
        drone_moving_to_station = move_drone;
        return;
    }

    float angle_diff = normalize_angle(target_rotation - drone->rotation.v[1]);
    float rotation_factor = DRONE_ROTATION_SPEED * delta_time;
    if (rotation_factor > 1.0f) rotation_factor = 1.0f;

    if (fabsf(angle_diff) < 0.01f) {
        drone->rotation.v[1] = target_rotation;
    } else {
        drone->rotation.v[1] += angle_diff * rotation_factor;
        drone->rotation.v[1] = normalize_angle(drone->rotation.v[1]);
    }

    float move_factor = DRONE_MOVE_SPEED * delta_time;
    drone->position.v[0] += dx * move_factor;
    drone->position.v[2] += dz * move_factor;
}



//beamlight
// Define a new PointLight for the beam

static float mining_accumulated = 0.0f;
static int cursor_mining_resource = -1;
static int drone_mining_resource = -1;
static T3DMat4FP *miningLightMatFP = NULL;
static bool cursor_is_mining = false;
static bool drone_is_mining = false;
rspq_block_t *miningDplLight;


// Function to load the beam as a point light
void load_point_light() {
  miningLightMatFP = malloc_uncached(sizeof(T3DMat4FP)); // Allocate matrix for mining light
  rspq_block_begin();
  miningDplLight = rspq_block_end();
}

void draw_mining_point_light(T3DVec3 cursor_pos, T3DVec3 look_dir) {
  if (!cursor_is_mining || cursor_mining_resource < 0) return;

  // Welder-like flicker effect - more dramatic intensity changes
  int flicker = rand() % 100;
  uint8_t brightness;

  if (flicker < 40) {
    brightness = 255;  // Full bright 40% of the time
  } else if (flicker < 60) {
    brightness = 200 + (rand() % 55);  // Medium-high flicker
  } else if (flicker < 80) {
    brightness = 120 + (rand() % 80);  // Medium flicker
  } else if (flicker < 95) {
    brightness = 50 + (rand() % 70);  // Dim flicker
  } else {
    brightness = 10 + (rand() % 40);  // Very dim - almost off
  }

  uint8_t light_color[4] = {brightness, brightness, brightness, 255};



  // Position light in front of cursor (like a headlamp)
  float offset_distance = 15.0f;
  T3DVec3 light_pos = {{
    cursor_pos.v[0] - look_dir.v[0] * offset_distance,
    cursor_pos.v[1] + 40.0f,  // Slightly above cursor
    cursor_pos.v[2] - look_dir.v[2] * offset_distance
  }};
  t3d_light_set_point(1, light_color, &light_pos, 300.0f, false);  // Much larger radius
}

// void draw_drone_mining_point_light(T3DVec3 drone_pos) {
//   if (!drone_is_mining || drone_mining_resource < 0) return;

//   // Welder-like flicker effect - more dramatic intensity changes
//   int flicker = rand() % 100;
//   uint8_t brightness;

//   if (flicker < 40) {
//     brightness = 255;  // Full bright 40% of the time
//   } else if (flicker < 60) {
//     brightness = 200 + (rand() % 55);  // Medium-high flicker
//   } else if (flicker < 80) {
//     brightness = 120 + (rand() % 80);  // Medium flicker
//   } else if (flicker < 95) {
//     brightness = 50 + (rand() % 70);  // Dim flicker
//   } else {
//     brightness = 10 + (rand() % 40);  // Very dim - almost off
//   }

//   uint8_t light_color[4] = {brightness, brightness, brightness, 255};

//   // Position light at drone position (drone hovers above resource)
//   T3DVec3 light_pos = {{
//     drone_pos.v[0],
//     drone_pos.v[1] + 20.0f,  // Slightly above drone
//     drone_pos.v[2]
//   }};
//   t3d_light_set_point(2, light_color, &light_pos, 300.0f, false);  // Use light index 2
// }

static void mine_resource(Entity *entity, Entity *resource, float delta_time) { // this seems slower
    float amount = delta_time * MINING_RATE;
    mining_accumulated += amount;

    // Only transfer whole units
    if (mining_accumulated >= 1.0f) {
        int transfer = (int)mining_accumulated;
        resource->value -= transfer;
        cursor_resource_val += transfer;
        mining_accumulated -= transfer;
        spawn_mining_sparks(resource->position);
    }

    resource_val = resource->value;


    if (resource->value <= 0) {
        mining_accumulated = 0.0f;  // Reset accumulator
        resource->value = 0;
        resource->color = COLOR_ASTEROID;
        entity->value += 5; // replenish some energy on resource depletion
        reset_entity(resource, RESOURCE);
    }
}
static void drone_mine_resource(Entity *entity, Entity *resource, float delta_time) { // this seems slower
    float amount = delta_time * DRONE_MINING_RATE;
    drone_mining_accumulated += amount;

    // Only transfer whole units
    if (drone_mining_accumulated >= 1.0f && drone_resource_val < DRONE_MAX_RESOURCES) {
        int transfer = (int)drone_mining_accumulated;
        resource->value -= transfer;
        // entity->value += transfer;
        drone_resource_val += transfer;
        entity->value = drone_resource_val;
        drone_mining_accumulated -= transfer;
        spawn_mining_sparks(entity->position);
    }

    resource_val = resource->value;

    if (resource->value <= 0) {
        drone_mining_accumulated = 0.0f;  // Reset accumulator
        resource->value = 0;
        resource->color = COLOR_ASTEROID;
        reset_entity(resource, RESOURCE);
    }
}

static void check_cursor_resource_collisions(Entity *cursor, Entity *resources, int count, float delta_time) {
    cursor_mining_resource = -1;
    cursor_is_mining = false;

    for (int i = 0; i < count; i++) {
        if (check_entity_intersection(cursor, &resources[i]) && (cursor_resource_val < CURSOR_RESOURCE_CAPACITY)) {
            resource_intersection = true;
            cursor_is_mining = true;

            // Flicker both resource and cursor color - welder effect
            int flicker = rand() % 100;
            if (flicker < 10) {
                cursor->color = RGBA32(0, 0, 0, 255);  // Cursor flashes white too
            } else if (flicker < 40) {
                resources[i].color = RGBA32(255, 255, 255, 255);  // White flash
                cursor->color = RGBA32(100, 100, 100, 255);  // Cursor flashes white too
            } else if (flicker < 70) {
                resources[i].color = RGBA32(200, 200, 200, 255);  // Light gray
                cursor->color = RGBA32(220, 220, 220, 255);  // Cursor light gray
            } else {
                resources[i].color = COLOR_RESOURCE;  // Normal color
                cursor->color = COLOR_CURSOR;  // Cursor normal color
            }

            play_sfx();
            cursor_mining_resource = i;
            mine_resource(cursor, &resources[i], delta_time);
            break;  // Only mine one at a time
        }
    }
}

static T3DVec3 last_drone_position = {{0.0f, 0.0f, 0.0f}};
static void check_drone_resource_collisions(Entity *entity, Entity *resources, int count, float delta_time) {
    drone_mining_resource = -1;
    drone_collecting_resource = false;  // Reset flag each frame
    drone_is_mining = false;

    for (int i = 0; i < count; i++) {
        if (check_entity_intersection(entity, &resources[i]) && drone_resource_val < DRONE_MAX_RESOURCES) {
            resource_intersection = true;
            drone_collecting_resource = true;
            drone_is_mining = true;
            entity->position.v[1] = resources[i].position.v[1] + 5.0f; // hover above resource
            entity->position.v[0] = resources[i].position.v[0];
            entity->position.v[2] = resources[i].position.v[2];

            // Flicker resource color between white and normal - welder effect
            int flicker = rand() % 100;
            if (flicker < 40) {
                resources[i].color = RGBA32(111, 239, 255, 255);  // White flash 27, 154, 170, 255)
            } else if (flicker < 70) {
                resources[i].color = RGBA32(200, 200, 200, 255);  // Light gray
            } else {
                resources[i].color = COLOR_RESOURCE;  // Normal color
            }



            play_sfx();
            drone_mining_resource = i;
            drone_mine_resource(entity, &resources[i], delta_time);

            break;  // Only mine one at a time
        }
    }
}

static void check_drone_station_collisions(Entity *drone, Entity *station, int count) {
        if (check_entity_intersection(drone, station)) {
            station->value += drone_resource_val;
            drone_resource_val = 0;
            drone->value = drone_resource_val;
            // start_entity_color_flash(station, RGBA32(0, 255, 0, 255), 0.5f);
        }

}
static void check_drone_cursor_collisions(Entity *drone, Entity *cursor, int count) {
        if (check_entity_intersection(drone, cursor)) {
            cursor->value += drone_resource_val;
            drone_resource_val = 0;
            drone->value = drone_resource_val;
            // start_entity_color_flash(cursor, RGBA32(0, 255, 0, 255), 0.5f);
        }
}


static void reset_resource_colors(Entity *resources, int count) {
    for (int i = 0; i < count; i++) {
        if (i != highlighted_resource && i != cursor_mining_resource && i != drone_mining_resource) {
            resources[i].color = COLOR_RESOURCE;
        } else if (resources[i].value <= 0) {
            resources[i].color = COLOR_ASTEROID;
        }
    }
}


void check_cursor_station_collision(Entity *cursor, Entity *station) {
    if (check_entity_intersection(cursor, station)) {
        station->value += cursor_resource_val;
        cursor_resource_val = 0;
        // start_entity_color_flash(station, RGBA32(0, 255, 0, 255), 0.5f);
    }
}

static void update_tile_visibility(Entity *tile) {
    tile->position.v[0] = move_drone ? target_position.v[0] : 0.0f;
    tile->position.v[1] = move_drone ? 8.0f : 1000.0f;
    tile->position.v[2] = move_drone ? target_position.v[2] : 0.0f;
}



// =============================================================================
// Input
// =============================================================================

static int menu_input_delay = 0;

static void process_input(float delta_time, float *cam_yaw) {
    joypad_inputs_t joypad = joypad_get_inputs(JOYPAD_PORT_1);
    joypad_buttons_t held = joypad_get_buttons_held(JOYPAD_PORT_1);
    joypad_buttons_t pressed = joypad_get_buttons_pressed(JOYPAD_PORT_1);

    // Toggle pause with START
    if (pressed.start) {
        game_paused = !game_paused;
        menu_selection = 0;
        menu_input_delay = 0;
        return;
    }

    if (game_paused) {
        // Decrease delay timer
        if (menu_input_delay > 0) menu_input_delay--;

        // Navigate up
        if (menu_input_delay == 0 && (pressed.d_up || pressed.c_up || joypad.stick_y > 50)) {
            menu_selection--;
            if (menu_selection < 0) menu_selection = MENU_OPTIONS_COUNT - 1;
            menu_input_delay = 10;  // Delay before next input
        }

        // Navigate down
        if (menu_input_delay == 0 && (pressed.d_down || pressed.c_down || joypad.stick_y < -50)) {
            menu_selection++;
            if (menu_selection >= MENU_OPTIONS_COUNT) menu_selection = 0;
            menu_input_delay = 10;
        }

        // Select with A (no delay needed for button press)
        if (pressed.a) {
            switch (menu_selection) {
                case MENU_OPTION_RESUME:
                    game_paused = false;
                    break;
                case MENU_OPTION_CAMERA:
                    fps_mode = !fps_mode;
                    break;
                case MENU_OPTION_DEBUG:
                    render_debug = !render_debug;
                    break;
                case MENU_OPTION_AUDIO:
                    if (bgm_playing) {
                        stop_bgm();
                    } else {
                        play_bgm("rom:/bgm_loop.wav64");
                    }
                    break;
                case MENU_OPTION_30HZ:
                    limit30hz = !limit30hz;
                    break;
            }
        }
        return;
    }

    if (!fps_mode) {
        float rotation_speed = CAM_ROTATION_SPEED * delta_time;
        if (held.r) *cam_yaw -= rotation_speed;  // Rotate right
        if (held.z) *cam_yaw += rotation_speed;  // Rotate left
    }

    if (!held.r && pressed.c_up) {
        fps_mode = !fps_mode;
    }

    if (pressed.l) reset_fps_stats();


    if (pressed.d_up) render_debug = !render_debug;
    // if (pressed.d_down) spawn_explosion(cursor_position);


    drone_heal = held.b;

    if (pressed.b && cursor_entity) {
        target_rotation = cursor_entity->rotation.v[1];
        // target_position = cursor_position;
        float offset_distance = 30.0f;  // How far in front of cursor
        target_position.v[0] = cursor_position.v[0] - cursor_look_direction.v[0] * offset_distance;
        target_position.v[1] = cursor_position.v[1];
        target_position.v[2] = cursor_position.v[2] - cursor_look_direction.v[2] * offset_distance;
        move_drone = true;
        drone_moving_to_resource = move_drone;
        drone_moving_to_station = false;
    }

    if (pressed.c_down && cursor_entity) {
        // move drone to station
        target_rotation = 0.0f;
        target_position.v[0] = 0.0f;
        target_position.v[2] = 0.0f;
        move_drone = true;

        drone_moving_to_station = move_drone;
        drone_moving_to_resource = false;
    }
}

static void draw_entity_health_bar(Entity *entity, float max_value, int y_offset, const char *label, int last_damage) {
    if (entity->value < 0) entity->value = 0;

    int x = 10;
    int y = 10 + y_offset;
    int bar_width = 40;
    int bar_height = 4;

    float health_percent = entity->value / max_value;

    if (health_percent > 1.0f) health_percent = 1.0f;
    if (health_percent < 0.0f) health_percent = 0.0f;

    int fill_width = (int)(bar_width * health_percent);

    color_t bar_color;
    if (health_percent > 0.6f) {
        bar_color = RGBA32(0, 255, 0, 255);
    } else if (health_percent > 0.3f) {
        bar_color = RGBA32(255, 255, 0, 255);
    } else {
        bar_color = RGBA32(255, 0, 0, 255);
    }

    rdpq_sync_pipe();
    rdpq_mode_combiner(RDPQ_COMBINER_FLAT);

    if (fill_width > 0) {
        rdpq_set_prim_color(bar_color);
        rdpq_fill_rectangle(x, y, x + fill_width, y + bar_height);
    }

    if (fill_width < bar_width) {
        rdpq_set_prim_color(RGBA32(50, 50, 50, 255));
        rdpq_fill_rectangle(x + fill_width, y, x + bar_width, y + bar_height);
    }

    // Label with damage
    rdpq_sync_pipe();

    rdpq_text_printf(
        &(rdpq_textparms_t){
            .char_spacing = 1,
        },
        FONT_CUSTOM,
        x + bar_width + 5,
        y + bar_height,
        "%s -%d",
        label,
        last_damage);
}


static void draw_triangle_indicator(int x, int y) {
    rdpq_set_prim_color(RGBA32(255, 165, 0, 255));  // Orange

    // Bigger triangle pointing right (10 pixels tall)
    rdpq_fill_rectangle(x, y + 4, x + 2, y + 6);
    rdpq_fill_rectangle(x + 2, y + 3, x + 4, y + 7);
    rdpq_fill_rectangle(x + 4, y + 2, x + 6, y + 8);
    rdpq_fill_rectangle(x + 6, y + 1, x + 8, y + 9);
    rdpq_fill_rectangle(x + 8, y, x + 10, y + 10);
}

static void draw_circle_indicator(int x, int y) {
    rdpq_set_prim_color(RGBA32(0, 255, 25, 255));  // blue-green
    //circle   (15 pixels tall) // can we make it smoother?
    rdpq_fill_rectangle(x + 4, y + 2, x + 6, y + 8);
    rdpq_fill_rectangle(x + 2, y + 4, x + 8, y + 6);
    rdpq_fill_rectangle(x + 4, y, x + 6, y + 2);
    rdpq_fill_rectangle(x + 2, y + 2, x + 8, y + 4);
    rdpq_fill_rectangle(x, y + 4, x + 10, y + 6);
    rdpq_fill_rectangle(x + 2, y + 6, x + 8, y + 8);
    rdpq_fill_rectangle(x + 4, y + 8, x + 6, y + 10);
}


static void draw_station_indicator(int x, int y) {
    rdpq_set_prim_color(RGBA32(244, 0, 125, 255));  // green
    //(10 pixels tall) // can we make this an lowercase t  with a square at the top?

    rdpq_fill_rectangle(x + 4, y, x + 6, y + 10);
    rdpq_fill_rectangle(x + 2, y, x + 8, y + 2);

}

static void draw_entity_resource_bar(int resource_val, float max_value, int y_offset, const char *label, bool show_triangle) {
    int bar_width = 40;
    int bar_height = 4;

    // Position in lower right
    int x = SCREEN_WIDTH - bar_width - 80;
    int y = SCREEN_HEIGHT - y_offset;

    float resource_percent = resource_val / max_value;

    if (resource_percent > 1.0f) resource_percent = 1.0f;
    if (resource_percent < 0.0f) resource_percent = 0.0f;

    int fill_width = (int)(bar_width * resource_percent);

    color_t bar_color = RGBA32(0, 191, 255, 255);

    rdpq_sync_pipe();
    rdpq_mode_combiner(RDPQ_COMBINER_FLAT);

    // Draw resource fill
    if (fill_width > 0) {
        rdpq_set_prim_color(bar_color);
        rdpq_fill_rectangle(x, y, x + fill_width, y + bar_height);
    }

    // Draw empty portion in gray
    if (fill_width < bar_width) {
        rdpq_set_prim_color(RGBA32(50, 50, 50, 255));
        rdpq_fill_rectangle(x + fill_width, y, x + bar_width, y + bar_height);
    }

    // Label
    rdpq_sync_pipe();
    rdpq_text_printf(NULL, FONT_CUSTOM, x + bar_width + 5, y + bar_height, "%s", label);

    int icon_x = x - 20;
    int icon_y = y - 6;

    // Triangle indicator only for drone when collecting
    if (show_triangle && blink_timer < 10) {
        if (drone_collecting_resource ) {
            rdpq_sync_pipe();
            rdpq_set_mode_standard();
            rdpq_mode_combiner(RDPQ_COMBINER1((TEX0, 0, PRIM, 0), (TEX0, 0, PRIM, 0)));
            rdpq_set_prim_color(RGBA32(255, 149, 5, 255));  // Saffron tint

            rdpq_mode_alphacompare(1);  // Enable transparency
            rdpq_sprite_blit(drill_icon, icon_x, icon_y, &(rdpq_blitparms_t){
                .scale_x = 0.5f,  // Half size
                .scale_y = 0.5f
            });
            drone_moving_to_resource = false;
            drone_moving_to_station = false;
        } else if (drone_moving_to_resource) {
            rdpq_sync_pipe();
            rdpq_set_mode_standard();
            rdpq_mode_combiner(RDPQ_COMBINER1((TEX0, 0, PRIM, 0), (TEX0, 0, PRIM, 0)));
            rdpq_set_prim_color(RGBA32(137, 252, 0, 255));  // Lime

            rdpq_mode_alphacompare(1);  // Enable transparency
            rdpq_sprite_blit(tile_icon, icon_x, icon_y, &(rdpq_blitparms_t){
                .scale_x = 0.0525f,  // Half size
                .scale_y = 0.0525f
            });
            drone_collecting_resource = false;
            drone_moving_to_station = false;
        } else if (drone_moving_to_station) {
            rdpq_sync_pipe();
            rdpq_set_mode_standard();
            rdpq_mode_combiner(RDPQ_COMBINER1((TEX0, 0, PRIM, 0), (TEX0, 0, PRIM, 0)));
            rdpq_set_prim_color(RGBA32(27, 154, 170, 255));  // Red tint

            rdpq_mode_alphacompare(1);  // Enable transparency
            rdpq_sprite_blit(station_icon, icon_x, icon_y, &(rdpq_blitparms_t){
                .scale_x = 0.5f,  // Half size
                .scale_y = 0.5f
            });
            drone_collecting_resource = false;
            drone_moving_to_resource = false;
        }
    } else if (show_triangle) {
        rdpq_sync_pipe();
        rdpq_set_mode_standard();
        rdpq_mode_combiner(RDPQ_COMBINER1((TEX0, 0, PRIM, 0), (TEX0, 0, PRIM, 0)));
        rdpq_set_prim_color(RGBA32(220, 0, 115, 255));  // Red tint

        rdpq_mode_alphacompare(1);  // Enable transparency
        rdpq_sprite_blit(drone_icon, icon_x, icon_y, &(rdpq_blitparms_t){
            .scale_x = 1.0f,  // Half size
            .scale_y = 1.0f
        });
    } else {
        rdpq_sync_pipe();
        rdpq_set_mode_standard();
        rdpq_mode_combiner(RDPQ_COMBINER1((TEX0, 0, PRIM, 0), (TEX0, 0, PRIM, 0)));
        rdpq_set_prim_color(RGBA32(170, 170, 170, 255));  // Red tint

        rdpq_mode_alphacompare(1);  // Enable transparency
        rdpq_sprite_blit(drone_icon, icon_x, icon_y, &(rdpq_blitparms_t){
            .scale_x = 1.0f,  // Half size
            .scale_y = 1.0f
        });
    }
}

static void draw_info_bars(void) {
    draw_entity_health_bar(&entities[ENTITY_STATION], STATION_MAX_HEALTH, 0, "STATION", station_last_damage);
    draw_entity_health_bar(cursor_entity, CURSOR_MAX_HEALTH, 15, "CURSOR", cursor_last_damage);
    draw_entity_resource_bar(cursor_resource_val, CURSOR_RESOURCE_CAPACITY, 15, "CURSOR RES", false);
    draw_entity_resource_bar(drone_resource_val, DRONE_MAX_RESOURCES, 30, "DRONE RES", true);
}


// =============================================================================
// Rendering
// =============================================================================



static void setup_lighting(void) {

    rspq_block_t *dplLight; //

    uint8_t color_ambient[4] = {0x40, 0x40, 0x40, 0xFF};  // Moderately darker
    uint8_t color_directional[4] = {0xFF, 0xFF, 0xFF, 0xFF};

    T3DVec3 light_dir = {{300.0f, 100.0f, 1.0f}};
    t3d_vec3_norm(&light_dir);

    t3d_light_set_ambient(color_ambient);
    t3d_light_set_directional(0, color_directional, &light_dir);

    // Light count will be set in render_frame after all lights are configured
}


static void render_background(sprite_t *background, float cam_yaw) {
    float normalized_yaw = fmodf(cam_yaw, 360.0f);
    if (normalized_yaw < 0) normalized_yaw += 360.0f;

    int offset_x = (int)((normalized_yaw / 360.0f) * BG_WIDTH);

    rdpq_set_mode_copy(false);
    rdpq_sprite_blit(background, -offset_x, 0, NULL);

    // Wrap for seamless loop
    if (offset_x + SCREEN_WIDTH > BG_WIDTH) {
        rdpq_sprite_blit(background, BG_WIDTH - offset_x, 0, NULL);
    }
}


static void render_frame(T3DViewport *viewport, sprite_t *background, float cam_yaw) {
    culled_count = 0;
    rdpq_attach(display_get(), display_get_zbuf());

    // Panning background
    render_background(background, cam_yaw);

    // 3D setup
    t3d_frame_start();
    t3d_viewport_attach(viewport);
    t3d_screen_clear_depth();
    setup_lighting();

    // Add mining pointlights for cursor and/or drone
    int light_count = 1;  // Start with directional light

    // Cursor now uses color flickering instead of point light
    // Uncomment below to use point light instead:
    // if (cursor_is_mining && cursor_mining_resource >= 0) {
    //     draw_mining_point_light(cursor_position, cursor_look_direction);
    //     light_count++;
    // }

    // Drone now uses color flickering instead of point light

    t3d_light_set_count(light_count);

    // // Skip culling for specific entities
    bool entity_skip_culling[ENTITY_COUNT] = {false};
    entity_skip_culling[ENTITY_GRID] = true;     // Always draw grid
    entity_skip_culling[ENTITY_STATION] = false;  // Always draw station (optional)
    entity_skip_culling[ENTITY_CURSOR] = true;  // Always draw station (optional)

     // Draw particles

    draw_entities_culled(entities, ENTITY_COUNT, entity_skip_culling);
    draw_entities_culled(asteroids, ASTEROID_COUNT, NULL);
    draw_entities_culled(resources, RESOURCE_COUNT, NULL);

    draw_particles(viewport);


    draw_fps_display(); // remove this

    draw_info_bars();

    // Debug: Always show mining status
    rdpq_sync_pipe();
    rdpq_text_printf(NULL, FONT_BUILTIN_DEBUG_MONO, 10, 100,
                     "Mining:%d Res:%d Val:%d", cursor_is_mining ? 1 : 0, cursor_mining_resource, cursor_resource_val);

    if (render_debug) {
        render_debug_ui(cursor_position, entities, resources, RESOURCE_COUNT, culled_count, cursor_resource_val, drone_resource_val);
    }
    if (game_paused) {
        // draw_pause_menu();
     }

    rdpq_detach_show();
}

// =============================================================================
// Main
// =============================================================================


int main(void) {
    init_subsystems();
    T3DViewport viewport = t3d_viewport_create();

    sprite_t *background = sprite_load("rom:/bg1024.sprite");
    station_icon = sprite_load("rom:/station.sprite");
    drill_icon = sprite_load("rom:/drill.sprite");
    tile_icon = sprite_load("rom:/tile2.sprite");
    drone_icon = sprite_load("rom:/drone.sprite");
    ship_icon = sprite_load("rom:/ship.sprite");
    health_icon = sprite_load("rom:/health.sprite");

    entities[ENTITY_CURSOR] = create_entity("rom:/miner.t3dm", cursor_position, 0.562605f, COLOR_CURSOR, DRAW_TEXTURED_LIT, 10.0f);
    entities[ENTITY_CURSOR].value = CURSOR_MAX_HEALTH;
    entities[ENTITY_DRONE] = create_entity("rom:/drone.t3dm", (T3DVec3){{20.0f, DEFAULT_HEIGHT, 29.0f}}, 0.75f, COLOR_DRONE, DRAW_TEXTURED_LIT, 30.0f);
    entities[ENTITY_TILE] = create_entity("rom:/tile.t3dm", (T3DVec3){{0, 1000, 0}}, 1.0f, COLOR_TILE, DRAW_SHADED, 0.0f);
    entities[ENTITY_STATION] = create_entity("rom:/stationew.t3dm", (T3DVec3){{0, DEFAULT_HEIGHT, 0}}, 1.50f, COLOR_STATION, DRAW_TEXTURED_LIT, 30.0f);
    entities[ENTITY_STATION].value = STATION_MAX_HEALTH;

    init_asteroids(asteroids, ASTEROID_COUNT);
    init_resources(resources, RESOURCE_COUNT);

    entities[ENTITY_GRID] = create_entity("rom:/grid.t3dm", (T3DVec3){{0, 1, 0}}, 1.0f, COLOR_MAP, DRAW_SHADED, 0.00f);


    cursor_entity = &entities[ENTITY_CURSOR];

    float last_time = get_time_s() - (1.0f / 60.0f);
    float cam_yaw = CAM_ANGLE_YAW;

    load_point_light(); // Initialize mining light (once)

    wav64_open(&sfx_mining, "rom:/ploop.wav64"); //mining sound effect
    play_bgm("rom:/bgm_loop.wav64"); // Start background music

    for (;;) {
        float current_time = get_time_s();
        float delta_time = current_time - last_time;
        last_time = current_time;

        joypad_poll();
        process_input(delta_time, &cam_yaw);

        if (!game_paused) {
            update_cursor(delta_time, cam_yaw);
            update_camera(&viewport, cam_yaw, delta_time, cursor_position, fps_mode, cursor_entity);
            update_tile_visibility(&entities[ENTITY_TILE]);

            if (move_drone) {
                move_drone_elsewhere(&entities[ENTITY_DRONE], delta_time, false);
            }

            update_asteroids(asteroids, ASTEROID_COUNT, delta_time);
            update_resources(resources, RESOURCE_COUNT, delta_time);

            // rotate_entity(&entities[ENTITY_STATION], delta_time, 0.250f);

            update_particles(delta_time);

            // Update all entity matrices in one batch
            update_entity_matrices(entities, ENTITY_COUNT);
            update_entity_matrices(asteroids, ASTEROID_COUNT);
            update_entity_matrices(resources, RESOURCE_COUNT);

            // Reset colors before collision checks
            reset_resource_colors(resources, RESOURCE_COUNT);

            // Collision detection - order from most frequent to least
            check_cursor_resource_collisions(&entities[ENTITY_CURSOR], resources, RESOURCE_COUNT, delta_time);
            check_cursor_station_collision(&entities[ENTITY_CURSOR], &entities[ENTITY_STATION]);
            check_cursor_asteroid_collisions(&entities[ENTITY_CURSOR], asteroids, ASTEROID_COUNT);

            check_drone_resource_collisions(&entities[ENTITY_DRONE], resources, RESOURCE_COUNT, delta_time);
            check_drone_station_collisions(&entities[ENTITY_DRONE], &entities[ENTITY_STATION], 1);

            if (drone_heal) {
                check_drone_cursor_collisions(&entities[ENTITY_DRONE], &entities[ENTITY_CURSOR], 1);
            }

            check_station_asteroid_collisions(&entities[ENTITY_STATION], asteroids, ASTEROID_COUNT, delta_time);

            update_color_flashes(delta_time);
            update_fps_stats(delta_time);

            blink_timer++; // for indicators
            if (blink_timer > 20) blink_timer = 0;
        }

         // Render while RSP/RDP are working
        render_frame(&viewport, background, cam_yaw);

        // Do audio update while waiting for rendering
        update_audio();
        rspq_wait();  // Wait for RSP to finish
    }

    // Cleanup
    cleanup_particles();

    sprite_free(background);
    sprite_free(station_icon);
    sprite_free(drill_icon);
    sprite_free(tile_icon);
    sprite_free(drone_icon);
    sprite_free(ship_icon);
    // sprite_free(health_icon);
    wav64_close(&sfx_mining);

    stop_bgm();
    free_all_entities(entities, ENTITY_COUNT);
    free_all_entities(asteroids, ASTEROID_COUNT);
    free_all_entities(resources, RESOURCE_COUNT);
    t3d_destroy();

    return 0;
}