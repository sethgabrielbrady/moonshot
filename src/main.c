#include <libdragon.h>
#include <t3d/t3d.h>
#include <t3d/t3ddebug.h>
#include <rdpq.h>
#include <stdlib.h>

#include "types.h"
#include "constants.h"
#include "utils.h"
#include "entity.h"
#include "camera.h"
#include "asteroid.h"
#include "audio.h"
#include "debug.h"

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





static bool is_entity_in_frustum(Entity *entity, T3DVec3 cam_position, T3DVec3 cam_target, float fov_degrees) {
    // Direction from camera to entity
    float dx = entity->position.v[0] - cam_position.v[0];
    float dy = entity->position.v[1] - cam_position.v[1];
    float dz = entity->position.v[2] - cam_position.v[2];

    // Distance to entity
    float distance = sqrtf(dx * dx + dy * dy + dz * dz);
    if (distance < 0.001f) return true;  // Entity at camera position

    // Normalize direction to entity
    float inv_dist = 1.0f / distance;
    dx *= inv_dist;
    dy *= inv_dist;
    dz *= inv_dist;

    // Camera forward direction
    float fx = cam_target.v[0] - cam_position.v[0];
    float fy = cam_target.v[1] - cam_position.v[1];
    float fz = cam_target.v[2] - cam_position.v[2];

    float forward_len = sqrtf(fx * fx + fy * fy + fz * fz);
    if (forward_len < 0.001f) return true;

    float inv_forward = 1.0f / forward_len;
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

    // Distance culling
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
    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 3, GAMMA_NONE, FILTERS_RESAMPLE);
    // display_set_fps_limit(60);
    // display_set_fps_limit(30);
    rdpq_init();

    joypad_init();
    t3d_init((T3DInitParams){});
    rdpq_text_register_font(
        FONT_BUILTIN_DEBUG_MONO,
        rdpq_font_load_builtin(FONT_BUILTIN_DEBUG_MONO)
    );

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

static void clamp_cursor_position(void) {
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

    float stick_magnitude = sqrtf(joypad.stick_x * joypad.stick_x +
                                  joypad.stick_y * joypad.stick_y);

    if (fps_mode) {
        // FPS mode: thrust relative to cursor facing direction
        float stick_y = joypad.stick_y / 128.0f;
        float stick_x = joypad.stick_x / 128.0f;

        if (fabsf(stick_y) > 0.1f && cursor_entity) {
            // Forward/backward thrust in cursor facing direction
            float thrust_x = -sinf(cursor_entity->rotation.v[1]) * stick_y * 128.0f;
            float thrust_z = -cosf(cursor_entity->rotation.v[1]) * stick_y * 128.0f;
            cursor_velocity.v[0] += thrust_x * CURSOR_THRUST * delta_time;
            cursor_velocity.v[2] -= thrust_z * CURSOR_THRUST * delta_time;
        }

        // Optional: Strafe with stick X
        if (fabsf(stick_x) > 0.1f && cursor_entity) {
            // Strafe perpendicular to cursor facing direction
            float strafe_x = -cosf(cursor_entity->rotation.v[1]) * stick_x * 128.0f;
            float strafe_z = sinf(cursor_entity->rotation.v[1]) * stick_x * 128.0f;
            cursor_velocity.v[0] += strafe_x * CURSOR_THRUST * delta_time;
            cursor_velocity.v[2] -= strafe_z * CURSOR_THRUST * delta_time;
        }

    } else {
        // Isometric mode: rotate cursor to face stick direction
        if (stick_magnitude > CURSOR_DEADZONE && cursor_entity) {
            cursor_entity->rotation.v[1] = atan2f(-rotated_x, -rotated_z);

            cursor_look_direction.v[0] = -rotated_x / stick_magnitude;
            cursor_look_direction.v[2] = rotated_z / stick_magnitude;
        }

        // Thrust in joystick direction
        if (stick_magnitude > CURSOR_DEADZONE) {
            cursor_velocity.v[0] += rotated_x * CURSOR_THRUST * delta_time;
            cursor_velocity.v[2] -= rotated_z * CURSOR_THRUST * delta_time;
        }
    }

    // Apply drag
    float drag = CURSOR_DRAG * delta_time;
    if (drag > 1.0f) drag = 1.0f;

    cursor_velocity.v[0] *= (1.0f - drag);
    cursor_velocity.v[2] *= (1.0f - drag);

    // Clamp velocity to max speed
    float speed = sqrtf(cursor_velocity.v[0] * cursor_velocity.v[0] +
                        cursor_velocity.v[2] * cursor_velocity.v[2]);
    if (speed > CURSOR_MAX_SPEED) {
        float scale = CURSOR_MAX_SPEED / speed;
        cursor_velocity.v[0] *= scale;
        cursor_velocity.v[2] *= scale;
    }

    // Stop completely if very slow
    if (speed < 0.1f) {
        cursor_velocity.v[0] = 0.0f;
        cursor_velocity.v[2] = 0.0f;
    }

    // Apply velocity to position
    cursor_position.v[0] += cursor_velocity.v[0] * delta_time;
    cursor_position.v[2] += cursor_velocity.v[2] * delta_time;

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
    float kinetic_energy = 0.5f * mass * asteroid->speed * asteroid->speed;

    return kinetic_energy * DAMAGE_MULTIPLIER;
}

static void check_station_asteroid_collisions(Entity *station, Entity *asteroids, int count) {
    for (int i = 0; i < count; i++) {
        if (check_entity_intersection(station, &asteroids[i])) {
            float damage = calculate_asteroid_damage(&asteroids[i]);
            if (damage <= MAX_DAMAGE) {
                damage = MAX_DAMAGE;
            }
            station->value -= damage;
            // Clamp to zero
            if (station->value < 0) {
                station->value = 0;
            }
            // Visual feedback
            start_entity_color_flash(station, RGBA32(255, 0, 0, 255), 0.5f);

            // Reset asteroid so it doesn't keep hitting
            reset_entity(&asteroids[i], ASTEROID);
        }
    }
}

static void check_cursor_asteroid_collisions(Entity *cursor, Entity *asteroids, int count) {
    for (int i = 0; i < count; i++) {
        if (check_entity_intersection(cursor, &asteroids[i])) {
            float damage = calculate_asteroid_damage(&asteroids[i]);
            if (damage <= MAX_DAMAGE) {
                damage = MAX_DAMAGE;
            }
            cursor->value -= damage;

            if (cursor->value < 0) {
                cursor->value = 0;
            }

            start_entity_color_flash(cursor, RGBA32(255, 0, 0, 255), 0.5f);

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


static float mining_accumulated = 0.0f;
static int mining_resource = -1;


static void mine_resource(Entity *entity, Entity *resource, float delta_time) { // this seems slower
    float amount = delta_time * MINING_RATE;
    mining_accumulated += amount;

    // Only transfer whole units
    if (mining_accumulated >= 1.0f) {
        int transfer = (int)mining_accumulated;
        resource->value -= transfer;
        // entity->value += transfer;
        cursor_resource_val += transfer;
        mining_accumulated -= transfer;
    }

    resource_val = resource->value;

    if (resource->value <= 0 || resource->value == 0) {
        mining_accumulated = 0.0f;  // Reset accumulator
        resource->value = 0;
        resource->color = COLOR_ASTEROID;
        // move_drone_elsewhere(drone, delta_time, true);
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
    }

    resource_val = resource->value;

    if (resource->value <= 0 || resource->value == 0) {
        drone_mining_accumulated = 0.0f;  // Reset accumulator
        resource->value = 0;
        resource->color = COLOR_ASTEROID;
        // move_drone_elsewhere(drone, delta_time, true);
        reset_entity(resource, RESOURCE);
    }
}

static void check_entity_resource_collisions(Entity *entity, Entity *resources, int count, float delta_time) {
    mining_resource = -1;

    for (int i = 0; i < count; i++) {
        if (check_entity_intersection(entity, &resources[i])) {
            resource_intersection = true;
            resources[i].color = RGBA32(255, 149, 5, 255);  // orange for mining
            play_sfx();
            mining_resource = i;
            mine_resource(entity, &resources[i], delta_time);
            break;  // Only mine one at a time
        }
    }
}

static T3DVec3 last_drone_position = {{0.0f, 0.0f, 0.0f}};
static void check_drone_resource_collisions(Entity *entity, Entity *resources, int count, float delta_time) {
    mining_resource = -1;

    for (int i = 0; i < count; i++) {
        if (check_entity_intersection(entity, &resources[i]) && drone_resource_val < DRONE_MAX_RESOURCES) {
            resource_intersection = true;
            entity->position.v[1] = resources[i].position.v[1] + 5.0f; // hover above resource
            entity->position.v[0] = resources[i].position.v[0];
            entity->position.v[2] = resources[i].position.v[2];
            resources[i].color = RGBA32(255, 149, 5, 255);  // orange for mining
            play_sfx();
            mining_resource = i;
            drone_mine_resource(entity, &resources[i], delta_time);

            break;  // Only mine one at a time
        }
    }
}

static void check_drone_station_collisions(Entity *drone, Entity *station, int count) {
        if (check_entity_intersection(drone, station)) {
            // entity_array[i].value += drone->value;
            // drone->value = 0;
            station->value += drone_resource_val;
            drone_resource_val = 0;
            start_entity_color_flash(station, RGBA32(0, 255, 0, 255), 0.5f);
        }

}
static void check_drone_cursor_collisions(Entity *drone, Entity *cursor, int count) {
        if (check_entity_intersection(drone, cursor)) {
            cursor->value += drone_resource_val;
            drone_resource_val = 0;
            start_entity_color_flash(cursor, RGBA32(0, 255, 0, 255), 0.5f);
        }
}




static void reset_resource_colors(Entity *resources, int count) {
    for (int i = 0; i < count; i++) {
        if (i != highlighted_resource && i != mining_resource) {
            resources[i].color = COLOR_RESOURCE;
        } else if (resources[i].value <= 0) {
            resources[i].color = COLOR_ASTEROID;
        }
    }
}


void check_cursor_station_collision(Entity *cursor, Entity *station) {
    if (check_entity_intersection(cursor, station)) {
        // station->value += cursor->value;
        // cursor->value = 0;
        station->value += cursor_resource_val;
        cursor_resource_val = 0;
        start_entity_color_flash(station, RGBA32(0, 255, 0, 255), 0.5f);
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

static void process_input(float delta_time, float *cam_yaw) {
    joypad_inputs_t joypad = joypad_get_inputs(JOYPAD_PORT_1);
    joypad_buttons_t held = joypad_get_buttons_held(JOYPAD_PORT_1);
    joypad_buttons_t pressed = joypad_get_buttons_pressed(JOYPAD_PORT_1);

    // if (held.r && fps_mode) {
    //     float rotation_speed = CAM_ROTATION_SPEED * delta_time;
    //     if (joypad.stick_x < 0) *cam_yaw -= rotation_speed;
    //     else if (joypad.stick_x > 0) *cam_yaw += rotation_speed;
    // }


    if (!fps_mode) {
        float rotation_speed = CAM_ROTATION_SPEED * delta_time;
        if (held.r) *cam_yaw -= rotation_speed;  // Rotate right
        if (held.z) *cam_yaw += rotation_speed;  // Rotate left
    }

    if (!held.r && pressed.c_up) {
        fps_mode = !fps_mode;
    }

    if (pressed.l) reset_fps_stats();

    // if (held.r) {
    //     if (pressed.c_up) teleport_to_position(-576.6f, 276.0f, cam_yaw, &cursor_position);
    //     else if (pressed.c_right) teleport_to_position(-489.6f, -470.0f, cam_yaw, &cursor_position);
    //     else if (pressed.c_down) teleport_to_position(690.6f, -519.0f, cam_yaw, &cursor_position);
    //     else if (pressed.c_left) teleport_to_position(730.6f, 378.0f, cam_yaw, &cursor_position);
    //     else if (pressed.z) teleport_to_position(0.0f, 0.0f, cam_yaw, &cursor_position);
    // }

    if (pressed.start) render_debug = !render_debug;

    drone_heal = false;
    if (held.c_left) {
       drone_heal = true;
    }
    if (pressed.c_left && cursor_entity) {
        target_rotation = cursor_entity->rotation.v[1];
        target_position = cursor_position;
        move_drone = true;
    }

    if (pressed.c_down && cursor_entity) {
        // move drone to station
        target_rotation = 0.0f;
        target_position.v[0] = 0.0f;
        target_position.v[2] = 0.0f;
        move_drone = true;

    }
}



static void draw_entity_health_bar(Entity *entity, float max_value, int y_offset, const char *label) {
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

    // Draw health fill only (no background overlap)
    if (fill_width > 0) {
        rdpq_set_prim_color(bar_color);
        rdpq_fill_rectangle(x, y, x + fill_width, y + bar_height);
    }

    // Draw empty portion in gray (background for missing health)
    if (fill_width < bar_width) {
        rdpq_set_prim_color(RGBA32(50, 50, 50, 255));
        rdpq_fill_rectangle(x + fill_width, y, x + bar_width, y + bar_height);
    }

    // Label
    rdpq_sync_pipe();
    rdpq_text_printf(NULL, FONT_BUILTIN_DEBUG_MONO, x + bar_width + 5, y + bar_height, "%s", label);

}

static void draw_health_bars(void) {
    draw_entity_health_bar(&entities[ENTITY_STATION], STATION_MAX_HEALTH, 0, "STATION");
    draw_entity_health_bar(cursor_entity, CURSOR_MAX_HEALTH, 15, "CURSOR");
    draw_entity_health_bar(&entities[ENTITY_DRONE], DRONE_MAX_RESOURCES, 30, "DRONE");
}

// =============================================================================
// Rendering
// =============================================================================

static void setup_lighting(void) {
    //uint8_t color_ambient[4] = {0x90, 0x90, 0x90, 0xFF};

    uint8_t color_ambient[4] = {0x60, 0x60, 0x60, 0xFF};  // Moderately darker
    uint8_t color_directional[4] = {0xFF, 0xFF, 0xFF, 0xFF};
    // uint8_t pointLightColorAmbient[4] = {20, 20, 20, 0xFF};
    // PointLight pointLight = {{{0.0f, 0.0f, 0.0f}}, 0.300f, {0x1F, 0x1F, 0x1F, 0xFF}};

    T3DVec3 light_dir = {{1.0f, 1.0f, 1.0f}};
    t3d_vec3_norm(&light_dir);

    t3d_light_set_ambient(color_ambient);
    t3d_light_set_directional(0, color_directional, &light_dir);
    t3d_light_set_count(1);
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


    // // Skip culling for specific entities
    bool entity_skip_culling[ENTITY_COUNT] = {false};
    entity_skip_culling[ENTITY_GRID] = true;     // Always draw grid
    entity_skip_culling[ENTITY_STATION] = false;  // Always draw station (optional)

    draw_entities_culled(entities, ENTITY_COUNT, entity_skip_culling);
    draw_entities_culled(asteroids, ASTEROID_COUNT, NULL);
    draw_entities_culled(resources, RESOURCE_COUNT, NULL);

    draw_health_bars();

    if (render_debug) {
        render_debug_ui(cursor_position, cursor_entity, move_drone, resource_val, entities, ENTITY_COUNT, resources, RESOURCE_COUNT, culled_count, cursor_resource_val, drone_resource_val);
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

    // decrease the size of asteroids and resources - increase station size
    entities[ENTITY_CURSOR] = create_entity("rom:/miner.t3dm", cursor_position, 0.5625f, COLOR_CURSOR, DRAW_TEXTURED_LIT, 10.0f);
    entities[ENTITY_CURSOR].value = CURSOR_MAX_HEALTH;

    entities[ENTITY_DRONE] = create_entity("rom:/drone.t3dm", (T3DVec3){{20.0f, DEFAULT_HEIGHT, 29.0f}}, 0.25f, COLOR_DRONE, DRAW_TEXTURED_LIT, 30.0f);
    entities[ENTITY_TILE] = create_entity("rom:/tile.t3dm", (T3DVec3){{0, 1000, 0}}, 1.0f, COLOR_TILE, DRAW_SHADED, 0.0f);
    entities[ENTITY_STATION] = create_entity("rom:/station2.t3dm", (T3DVec3){{0, DEFAULT_HEIGHT, 0}}, 1.00f, COLOR_STATION, DRAW_SHADED, 30.0f);
    entities[ENTITY_STATION].value = STATION_MAX_HEALTH;

    init_asteroids(asteroids, ASTEROID_COUNT);
    init_resources(resources, RESOURCE_COUNT);


    entities[ENTITY_GRID] = create_entity("rom:/grid.t3dm", (T3DVec3){{0, 1, 0}}, 1.0f, COLOR_MAP, DRAW_SHADED, 0.0f);
    cursor_entity = &entities[ENTITY_CURSOR];

    float last_time = get_time_s() - (1.0f / 60.0f);
    float cam_yaw = CAM_ANGLE_YAW;

    wav64_open(&sfx_mining, "rom:/ploop.wav64");
    // Start background music
    play_bgm("rom:/bgm_loop.wav64");

    for (;;) {
        float current_time = get_time_s();
        float delta_time = current_time - last_time;
        last_time = current_time;

        joypad_poll();
        process_input(delta_time, &cam_yaw);

        update_cursor(delta_time, cam_yaw);
        update_camera(&viewport, cam_yaw, delta_time, cursor_position, fps_mode, cursor_entity);

        update_tile_visibility(&entities[ENTITY_TILE]);
        update_audio();


        if (move_drone) {
            move_drone_elsewhere(&entities[ENTITY_DRONE], delta_time, false);
        }

        update_asteroids(asteroids, ASTEROID_COUNT, delta_time);
        update_resources(resources, RESOURCE_COUNT, delta_time);
        rotate_entity(&entities[ENTITY_STATION], delta_time, 0.250f);

        update_entity_matrices(entities, ENTITY_COUNT);
        update_entity_matrices(asteroids, ASTEROID_COUNT);
        update_entity_matrices(resources, RESOURCE_COUNT);

        reset_resource_colors(resources, RESOURCE_COUNT);

        check_entity_resource_collisions(&entities[ENTITY_CURSOR], resources, RESOURCE_COUNT, delta_time);
        check_drone_resource_collisions(&entities[ENTITY_DRONE], resources, RESOURCE_COUNT, delta_time);
        check_drone_station_collisions(&entities[ENTITY_DRONE], &entities[ENTITY_STATION], 1);
        if (drone_heal  ) {
            check_drone_cursor_collisions(&entities[ENTITY_DRONE], &entities[ENTITY_CURSOR], 1);
        }
        check_station_asteroid_collisions(&entities[ENTITY_STATION], asteroids, ASTEROID_COUNT);
        check_cursor_station_collision(&entities[ENTITY_CURSOR], &entities[ENTITY_STATION]);
        check_cursor_asteroid_collisions(&entities[ENTITY_CURSOR], asteroids, ASTEROID_COUNT);

        update_color_flashes(delta_time);
        update_fps_stats(delta_time);
        render_frame(&viewport, background, cam_yaw);
        update_audio();


    }

    stop_bgm();

    free_all_entities(entities, ENTITY_COUNT);
    free_all_entities(asteroids, ASTEROID_COUNT);
    free_all_entities(resources, RESOURCE_COUNT);
    t3d_destroy();

    return 0;
}