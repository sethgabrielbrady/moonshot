// =============================================================================
// AsteRisk - Main Game File (Refactored)
// =============================================================================

#include <libdragon.h>
#include <t3d/t3d.h>
#include <t3d/t3ddebug.h>
#include <rdpq.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

// Core modules
#include "types.h"
#include "constants.h"
#include "game_state.h"
#include "utils.h"

// Game systems
#include "entity.h"
#include "camera.h"
#include "spawner.h"
#include "audio.h"
#include "debug.h"
#include "particles.h"
#include "collision.h"
#include "input.h"
#include "ui.h"

// =============================================================================
// Entity Arrays (not in game_state - too large)
// =============================================================================


static Entity entities[ENTITY_COUNT];
static Entity asteroids[ASTEROID_COUNT];
static Entity resources[RESOURCE_COUNT];
static Entity *cursor_entity = NULL;



// Visibility arrays for culling
static bool asteroid_visible[ASTEROID_COUNT];
static bool resource_visible[RESOURCE_COUNT];
static bool decrease_money = false;




// =============================================================================
// Frustum Culling
// =============================================================================

static int culled_count = 0;

static bool is_entity_in_frustum(Entity *entity, T3DVec3 cam_position, T3DVec3 cam_target, float fov_degrees) {
    float dx = entity->position.v[0] - cam_position.v[0];
    float dy = entity->position.v[1] - cam_position.v[1];
    float dz = entity->position.v[2] - cam_position.v[2];

    float distance_sq = dx * dx + dy * dy + dz * dz;
    if (distance_sq < 0.000001f) return true;

    float inv_dist = fast_inv_sqrt(distance_sq);
    dx *= inv_dist;
    dy *= inv_dist;
    dz *= inv_dist;

    float fx = cam_target.v[0] - cam_position.v[0];
    float fy = cam_target.v[1] - cam_position.v[1];
    float fz = cam_target.v[2] - cam_position.v[2];

    float forward_len_sq = fx * fx + fy * fy + fz * fz;
    if (forward_len_sq < 0.000001f) return true;

    float inv_forward = fast_inv_sqrt(forward_len_sq);
    fx *= inv_forward;
    fy *= inv_forward;
    fz *= inv_forward;

    float dot = dx * fx + dy * fy + dz * fz;

    float half_fov_rad = T3D_DEG_TO_RAD(fov_degrees * 0.5f);
    float cos_threshold = cosf(half_fov_rad * 1.2f);

    if (dot < cos_threshold) {
        return false;
    }

    float distance = 1.0f / inv_dist;
    float radius = entity->scale * 50.0f;
    if (distance - radius > CAM_FAR_PLANE) {
        return false;
    }

    return true;
}

static void compute_visibility(Entity *entity_array, bool *visibility, int count) {
    for (int i = 0; i < count; i++) {
        visibility[i] = is_entity_in_frustum(&entity_array[i], camera.position, camera.target, CAM_DEFAULT_FOV);
    }
}

// =============================================================================
// Entity Drawing with Culling and Sorting
// =============================================================================

typedef struct {
    int index;
    float distance_sq;
} EntityDistance;

static int compare_distance(const void *a, const void *b) {
    const EntityDistance *ea = (const EntityDistance *)a;
    const EntityDistance *eb = (const EntityDistance *)b;
    if (ea->distance_sq < eb->distance_sq) return -1;
    if (ea->distance_sq > eb->distance_sq) return 1;
    return 0;
}

static void draw_entities_sorted(Entity *entity_array, int count, bool *skip_culling, bool *visibility) {
    static EntityDistance sorted_indices[64];
    int visible_count = 0;

    for (int i = 0; i < count && visible_count < 64; i++) {
        bool should_draw = false;

        if (skip_culling != NULL && skip_culling[i]) {
            should_draw = true;
        } else if (visibility != NULL && visibility[i]) {
            should_draw = true;
        } else if (visibility == NULL && is_entity_in_frustum(&entity_array[i], camera.position, camera.target, CAM_DEFAULT_FOV)) {
            should_draw = true;
        } else {
            culled_count++;
        }

        if (should_draw) {
            float dx = entity_array[i].position.v[0] - camera.position.v[0];
            float dz = entity_array[i].position.v[2] - camera.position.v[2];
            sorted_indices[visible_count].index = i;
            sorted_indices[visible_count].distance_sq = dx * dx + dz * dz;
            visible_count++;
        }
    }

    if (visible_count > 1) {
        qsort(sorted_indices, visible_count, sizeof(EntityDistance), compare_distance);
    }

    for (int i = 0; i < visible_count; i++) {
        draw_entity(&entity_array[sorted_indices[i].index]);
    }
}

// =============================================================================
// Cursor Scale by Distance
// =============================================================================

static void update_cursor_scale_by_distance(Entity *cursor, Entity *station, float delta_time) {
    float dx = cursor->position.v[0] - station->position.v[0];
    float dz = cursor->position.v[2] - station->position.v[2];
    float distance_sq = dx * dx + dz * dz;

    // Use fast inverse sqrt: distance = distance_sq * (1/sqrt(distance_sq))
    float distance = distance_sq * fast_inv_sqrt(distance_sq);

    const float START_DISTANCE = 40.0f;
    const float END_DISTANCE = 30.0f;
    const float MIN_SCALE = 0.05f;
    const float MAX_SCALE = 1.0f;

    float target_scale;
    if (distance >= START_DISTANCE) {
        target_scale = MAX_SCALE;
    } else if (distance <= END_DISTANCE) {
        target_scale = MIN_SCALE;
    } else {
        float t = (distance - END_DISTANCE) / (START_DISTANCE - END_DISTANCE);
        target_scale = MIN_SCALE + (MAX_SCALE - MIN_SCALE) * t;
    }

    float scale_speed = 2.0f;
    game.cursor_scale_multiplier += (target_scale - game.cursor_scale_multiplier) * scale_speed * delta_time;
}

// =============================================================================
// Drone Movement
// =============================================================================

static void move_drone_to_target(Entity *drone, float delta_time, bool base_return) {
    if (base_return) {
        game.drone_target_position.v[0] = 0.0f;
        game.drone_target_position.v[2] = 0.0f;
    }

    float dx = game.drone_target_position.v[0] - drone->position.v[0];
    float dz = game.drone_target_position.v[2] - drone->position.v[2];
    float distance_sq = dx * dx + dz * dz;

    if (distance_sq < DRONE_ARRIVE_THRESHOLD * DRONE_ARRIVE_THRESHOLD) {
        game.move_drone = false;
        game.drone_moving_to_resource = false;
        game.drone_moving_to_station = false;
        return;
    }

    float angle_diff = normalize_angle(game.drone_target_rotation - drone->rotation.v[1]);
    float rotation_factor = DRONE_ROTATION_SPEED * delta_time;
    if (rotation_factor > 1.0f) rotation_factor = 1.0f;

    if (fabsf(angle_diff) < 0.01f) {
        drone->rotation.v[1] = game.drone_target_rotation;
    } else {
        drone->rotation.v[1] += angle_diff * rotation_factor;
        drone->rotation.v[1] = normalize_angle(drone->rotation.v[1]);
    }

    float move_factor = DRONE_MOVE_SPEED * delta_time;
    drone->position.v[0] += dx * move_factor;
    drone->position.v[2] += dz * move_factor;
}

// =============================================================================
// Tile Visibility and Following
// =============================================================================

static void update_tile_visibility(Entity *tile) {
    if (game.drone_full && game.tile_following_resource >= 0) {
        game.tile_following_resource = -1;
    }

    if (game.tile_following_resource >= 0 && game.tile_following_resource < RESOURCE_COUNT) {
        tile->position.v[0] = resources[game.tile_following_resource].position.v[0];
        tile->position.v[1] = 8.0f;
        tile->position.v[2] = resources[game.tile_following_resource].position.v[2];
    } else if (game.move_drone) {
        tile->position.v[0] = game.drone_target_position.v[0];
        tile->position.v[1] = 8.0f;
        tile->position.v[2] = game.drone_target_position.v[2];
    } else {
        tile->position.v[0] = 0.0f;
        tile->position.v[1] = 1000.0f;
        tile->position.v[2] = 0.0f;
    }
}

static void check_tile_following_status(Entity *drone) {
    if (game.tile_following_resource >= 0 && game.tile_following_resource < RESOURCE_COUNT) {
        if (game.drone_mining_resource == game.tile_following_resource) {
            game.tile_following_resource = -1;
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
    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 3, GAMMA_NONE, FILTERS_RESAMPLE_ANTIALIAS);

    tv_type_t tv = get_tv_type();
    game.is_pal_system = (tv == TV_PAL);

     // Reinitialize display with correct settings for PAL if needed
    if (game.is_pal_system) {
        display_close();
        display_init(RESOLUTION_256x240, DEPTH_16_BPP, 3, GAMMA_NONE, FILTERS_RESAMPLE_ANTIALIAS);
        display_set_fps_limit(25);
        debugf("PAL system detected - 256x240 @ 25fps\n");
    } else {
        display_set_fps_limit(30);
        debugf("NTSC system detected - 320x240 @ 30fps\n");
    }

    // Reset FPS stats after setting limit
    reset_fps_stats();

    rdpq_init();
    joypad_init();
    t3d_init((T3DInitParams){});
    init_particles();

    rdpq_text_register_font(FONT_BUILTIN_DEBUG_MONO, rdpq_font_load_builtin(FONT_BUILTIN_DEBUG_MONO));

    custom_font = rdpq_font_load("rom:/Nebula.font64");
    icon_font = rdpq_font_load("rom:/IconBitOne.font64");

    rdpq_text_register_font(FONT_CUSTOM, custom_font);
    rdpq_text_register_font(FONT_ICON, icon_font);

    rdpq_font_style(custom_font, 0, &(rdpq_fontstyle_t){
        .color = RGBA32(255, 255, 255, 255),
    });

    audio_init(32000, 4);
    mixer_init(12);
    set_bgm_volume(0.5f);

    srand(get_ticks());

    init_game_state();
}

// =============================================================================
// Lighting Setup
// =============================================================================

static void setup_lighting(void) {
    uint8_t color_ambient[4] = {0x60, 0x60, 0x60, 0xFF};
    uint8_t color_directional[4] = {0xFF, 0xFF, 0xFF, 0xFF};

    T3DVec3 light_dir = {{300.0f, 100.0f, 1.0f}};
    t3d_vec3_norm(&light_dir);

    t3d_light_set_ambient(color_ambient);
    t3d_light_set_directional(0, color_directional, &light_dir);
}

// =============================================================================
// Background Rendering
// =============================================================================

static void render_background(sprite_t *background, float cam_yaw) {
    float normalized_yaw = fmodf(cam_yaw, 360.0f);
    if (normalized_yaw < 0) normalized_yaw += 360.0f;

    int offset_x = (int)(normalized_yaw * 2.84444444f);
    int screen_width = display_get_width();
    bool needs_wrap = (offset_x + screen_width > BG_WIDTH);

    rdpq_set_mode_copy(false);
    rdpq_sprite_blit(background, -offset_x, 0, NULL);

    if (needs_wrap) {
        rdpq_sprite_blit(background, BG_WIDTH - offset_x, 0, NULL);
    }
}

// =============================================================================
// UI Drawing
// =============================================================================

static void draw_cursor_fuel_bar() {
    int x = 7;
    int y = SCREEN_HEIGHT - 23;
    int bar_width = 40;
    int bar_height = 3;

    float fuel_percent = game.ship_fuel / CURSOR_MAX_FUEL;
    if (fuel_percent > 1.0f) fuel_percent = 1.0f;
    if (fuel_percent < 0.0f) fuel_percent = 0.0f;

    int fill_width = (int)(bar_width * fuel_percent);
    color_t bar_color = RGBA32(200, 0, 200, 255);

    rdpq_sync_pipe();
    rdpq_mode_combiner(RDPQ_COMBINER_FLAT);

    if (fill_width > 0) {
        rdpq_set_prim_color(bar_color);
        rdpq_fill_rectangle(x, y, x + fill_width, y + bar_height);
    }

    if (fill_width < bar_width) {
        rdpq_set_prim_color(RGBA32(110, 0, 110, 115));
        rdpq_fill_rectangle(x + fill_width, y, x + bar_width, y + bar_height);
    }
}

static void draw_entity_health_bar(Entity *entity, float max_value, int y_offset, const char *label, int cursor, bool station_entity) {
    if (entity->value < 0) entity->value = 0;

    int x = 6;
    int y = 10 + y_offset;
    int bar_width = 40;
    int bar_height = 3;

    if (cursor) {
        y = SCREEN_HEIGHT - 26;
    }

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


    if (station_entity) {
        rdpq_text_printf(&(rdpq_textparms_t){.char_spacing = 1}, FONT_CUSTOM,
                 x, y, ">");
        rdpq_text_printf(&(rdpq_textparms_t){.char_spacing = 1}, FONT_CUSTOM,
                 x + 5, y, "%d", game.accumulated_credits);
    } else {
        rdpq_sync_pipe();
        rdpq_mode_combiner(RDPQ_COMBINER_FLAT);

        if (fill_width > 0) {
            rdpq_set_prim_color(bar_color);
            rdpq_fill_rectangle(x, y, x + fill_width, y + bar_height);
        }

        if (fill_width < bar_width) {
            rdpq_set_prim_color(RGBA32(0, 155, 0, 105));
            rdpq_fill_rectangle(x + fill_width, y, x + bar_width, y + bar_height);
        }

        rdpq_text_printf(&(rdpq_textparms_t){.char_spacing = 1}, FONT_CUSTOM,
                x + bar_width + 5, y + bar_height, "%s", label);
    }
}

static void draw_entity_resource_bar(int resource_val, float max_value, int y_offset, const char *label, bool show_triangle) {
    int bar_width = 40;
    int bar_height = 3;

    int x = display_get_width() - bar_width - 2;
    int y = SCREEN_HEIGHT - y_offset;

    if (!show_triangle) {
        y = SCREEN_HEIGHT - 20;
        x = 8;
    }

    float resource_percent = resource_val / max_value;
    if (resource_percent > 1.0f) resource_percent = 1.0f;
    if (resource_percent < 0.0f) resource_percent = 0.0f;

    int fill_width = (int)(bar_width * resource_percent);
    color_t bar_color = RGBA32(0, 191, 255, 255);

    rdpq_sync_pipe();
    rdpq_mode_combiner(RDPQ_COMBINER_FLAT);

    if (!show_triangle) {
        if (fill_width > 0) {
            rdpq_set_prim_color(bar_color);
            rdpq_fill_rectangle(x, y, x + fill_width, y + bar_height);
        }

        if (fill_width < bar_width) {
            rdpq_set_prim_color(RGBA32(50, 130, 165, 105));
            rdpq_fill_rectangle(x + fill_width, y, x + bar_width, y + bar_height);
        }
    }

    int icon_x = x - 20;
    int icon_y = y - 6;

    if (show_triangle) {
        icon_x = display_get_width() - 30;
        int action_x = icon_x - 8;
        int action_y = icon_y + 10;

        rdpq_sync_pipe();
        rdpq_set_mode_standard();
        rdpq_mode_combiner(RDPQ_COMBINER1((TEX0, 0, PRIM, 0), (TEX0, 0, PRIM, 0)));

        if (game.drone_resource_val >= DRONE_MAX_RESOURCES) {
            rdpq_set_prim_color(RGBA32(220, 0, 115, 255));
            game.drone_collecting_resource = false;
        } else {
            rdpq_set_prim_color(RGBA32(0, 255, 191, 255));
        }

        rdpq_mode_alphacompare(1);
        rdpq_sprite_blit(drone_icon, icon_x, icon_y, &(rdpq_blitparms_t){
            .scale_x = 1.0f,
            .scale_y = 1.0f
        });

        if (game.blink_timer < 10) {
            if (game.drone_collecting_resource) {
                rdpq_sync_pipe();
                rdpq_set_mode_standard();
                rdpq_mode_combiner(RDPQ_COMBINER1((TEX0, 0, PRIM, 0), (TEX0, 0, PRIM, 0)));
                rdpq_set_prim_color(RGBA32(220, 0, 115, 255));
                rdpq_mode_alphacompare(1);
                rdpq_sprite_blit(drill_icon, action_x, action_y, &(rdpq_blitparms_t){
                    .scale_x = 1.0f, .scale_y = 1.0f
                });
                game.drone_moving_to_resource = false;
                game.drone_moving_to_station = false;
            } else if (game.drone_moving_to_resource) {
                rdpq_sync_pipe();
                rdpq_set_mode_standard();
                rdpq_mode_combiner(RDPQ_COMBINER1((TEX0, 0, PRIM, 0), (TEX0, 0, PRIM, 0)));
                rdpq_set_prim_color(RGBA32(255, 128, 13, 255));
                rdpq_mode_alphacompare(1);
                rdpq_sprite_blit(tile_icon, action_x, action_y, &(rdpq_blitparms_t){
                    .scale_x = 1.0f, .scale_y = 1.0f
                });
                game.drone_collecting_resource = false;
                game.drone_moving_to_station = false;
            } else if (game.drone_moving_to_station) {
                rdpq_sync_pipe();
                rdpq_set_mode_standard();
                rdpq_mode_combiner(RDPQ_COMBINER1((TEX0, 0, PRIM, 0), (TEX0, 0, PRIM, 0)));
                rdpq_set_prim_color(RGBA32(200, 200, 200, 255));
                rdpq_mode_alphacompare(1);
                rdpq_sprite_blit(station_icon, action_x, action_y, &(rdpq_blitparms_t){
                    .scale_x = 1.0f, .scale_y = 1.0f
                });
                game.drone_collecting_resource = false;
                game.drone_moving_to_resource = false;

            } else if (game.drone_heal) {
                rdpq_sync_pipe();
                rdpq_set_mode_standard();
                rdpq_mode_combiner(RDPQ_COMBINER1((TEX0, 0, PRIM, 0), (TEX0, 0, PRIM, 0)));
                rdpq_set_prim_color(RGBA32(0, 180, 20, 255));
                rdpq_mode_alphacompare(1);
                rdpq_sprite_blit(health_icon, action_x, action_y, &(rdpq_blitparms_t){
                    .scale_x = 0.50f, .scale_y = 0.50f
                });
                game.drone_collecting_resource = false;
                game.drone_moving_to_resource = false;
                game.drone_moving_to_station = false;
            } else if (game.drone_full) {
                rdpq_sync_pipe();
                rdpq_set_mode_standard();
                rdpq_mode_combiner(RDPQ_COMBINER1((TEX0, 0, PRIM, 0), (TEX0, 0, PRIM, 0)));
                rdpq_set_prim_color(RGBA32(137, 242, 0, 255));
                rdpq_mode_alphacompare(1);
                rdpq_sprite_blit(drone_full_icon, action_x, action_y, &(rdpq_blitparms_t){
                    .scale_x = 1.0f, .scale_y = 1.0f
                });
                game.drone_collecting_resource = false;
                game.drone_moving_to_resource = false;
                game.drone_moving_to_station = false;
            }
        }
    }
}

static void draw_countdown(void) {
    int center_x = display_get_width() / 2;
    int center_y = SCREEN_HEIGHT / 2;

    // Get the current countdown number (3, 2, 1, or "GO!")
    int count = (int)game.countdown_timer + 1;

    if (count > 0 && count <= 3) {
        rdpq_text_printf(&(rdpq_textparms_t){.char_spacing = 1}, FONT_CUSTOM,
                 center_x - 5, center_y, "%d", count);
    } else if (game.countdown_timer > -0.5f) {
        // Show "GO!" briefly after countdown hits 0
        rdpq_text_printf(&(rdpq_textparms_t){.char_spacing = 1}, FONT_CUSTOM,
                 center_x - 12, center_y, "GO!");
    }
}

static void draw_game_timer(void) {
    // Convert game_time (seconds) to MM:SS format
    int total_seconds = (int)game.game_time;
    int minutes = total_seconds / 60;
    int seconds = total_seconds % 60;

    // Position in bottom right corner
    int x = display_get_width() - 38;
    int y = SCREEN_HEIGHT - 10;

    // Draw difficulty multiplier above the timer
    rdpq_text_printf(&(rdpq_textparms_t){.char_spacing = 1}, FONT_CUSTOM,
             x, y - 10, "x%.2f", game.difficulty_multiplier);

    // Draw timer
    rdpq_text_printf(&(rdpq_textparms_t){.char_spacing = 1}, FONT_CUSTOM,
             x, y, "%d:%02d", minutes, seconds);
}

static void draw_info_bars(void) {
    draw_entity_health_bar(&entities[ENTITY_STATION], STATION_MAX_HEALTH, 0, "S", 0, true);
    draw_cursor_fuel_bar();
    draw_entity_health_bar(cursor_entity, CURSOR_MAX_HEALTH, 20, "Procyon", 1, false);
    draw_entity_resource_bar(game.cursor_resource_val, CURSOR_RESOURCE_CAPACITY, 25, "Procyon", false);
    draw_entity_resource_bar(game.drone_resource_val, DRONE_MAX_RESOURCES, 225, "PUP", true);
    draw_game_timer();
}


// update station value to drop resources over time
static float fuel_drain_accumulated = 0.0f;
static void update_ship_fuel(float delta_time) {
    const float FUEL_DRAIN_RATE = 1.50f; // resources per second

    fuel_drain_accumulated += FUEL_DRAIN_RATE * delta_time;

    // Only drain whole units
    if (fuel_drain_accumulated >= 1.0f) {
        int drain = (int)fuel_drain_accumulated;
        game.ship_fuel -= drain;
        fuel_drain_accumulated -= drain;

        if (game.ship_fuel < 0) {
            game.ship_fuel = 0;
            game.disabled_controls = true;
        }
        if (game.ship_fuel > 0 && entities[ENTITY_CURSOR].value > 0) {
            game.disabled_controls = false;
        }
    }
}


// =============================================================================
// Frame Rendering
// =============================================================================

static void render_frame(T3DViewport *viewport, sprite_t *background, float cam_yaw, float delta_time) {
    culled_count = 0;
    rdpq_attach(display_get(), display_get_zbuf());

    if (game.render_background_enabled) {
        render_background(background, cam_yaw);
    } else {
        rdpq_set_mode_fill(RGBA32(0, 0, 0, 255));
        rdpq_fill_rectangle(0, 0, display_get_width(), display_get_height());
    }


    t3d_frame_start();
    t3d_viewport_attach(viewport);
    t3d_screen_clear_depth();
    setup_lighting();

    int light_count = 1;
    t3d_light_set_count(light_count);

    bool entity_skip_culling[ENTITY_COUNT] = {false};
    entity_skip_culling[ENTITY_GRID] = true;
    entity_skip_culling[ENTITY_CURSOR] = true;

    // Draw main entities
    for (int i = 0; i < ENTITY_COUNT; i++) {
        if (i == ENTITY_STATION || i == ENTITY_GRID) continue;

        // Cursor with distance-based scale
        if (i == ENTITY_CURSOR && game.cursor_scale_multiplier != 1.0f) {
            float original_scale = entities[i].scale;
            T3DMat4 temp_mat;
            t3d_mat4_from_srt_euler(&temp_mat,
                (float[3]){original_scale * game.cursor_scale_multiplier,
                           original_scale * game.cursor_scale_multiplier,
                           original_scale * game.cursor_scale_multiplier},
                (float[3]){entities[i].rotation.v[0], entities[i].rotation.v[1], entities[i].rotation.v[2]},
                entities[i].position.v);
            t3d_mat4_to_fixed(entities[i].matrix, &temp_mat);
            draw_entity(&entities[i]);
            entities[i].scale = original_scale;
            continue;
        }

        // Tile with animated scale
        if (i == ENTITY_TILE && (game.drone_moving_to_station || game.move_drone || game.tile_following_resource >= 0)) {
            color_t original_color = entities[i].color;
            float original_scale = entities[i].scale;

            if (game.drone_moving_to_station) {
                entities[i].color = RGBA32(255, 0, 255, 255);
            } else if (game.drone_heal) {
                entities[i].color = RGBA32(0, 255, 0, 155);
            } else if (game.tile_following_resource) {
                entities[i].color = COLOR_TILE;
            }

            T3DMat4 temp_mat;
            t3d_mat4_from_srt_euler(&temp_mat,
                (float[3]){original_scale * game.tile_scale_multiplier, original_scale, original_scale * game.tile_scale_multiplier},
                (float[3]){entities[i].rotation.v[0], entities[i].rotation.v[1], entities[i].rotation.v[2]},
                entities[i].position.v);
            t3d_mat4_to_fixed(entities[i].matrix, &temp_mat);
            draw_entity(&entities[i]);
            entities[i].color = original_color;
            entities[i].scale = original_scale;
            continue;
        }

        // Deflection ring - only draw when active
        if (i == ENTITY_DEFLECT_RING) {
            if (game.deflect_active) {
                float deflect_scale = DEFLECT_RADIUS / 15.0f;

                T3DMat4 deflect_mat;
                t3d_mat4_from_srt_euler(&deflect_mat,
                    (float[3]){deflect_scale, 0.5f, deflect_scale},
                    (float[3]){0, 0, 0},
                    (float[3]){entities[ENTITY_CURSOR].position.v[0],
                            entities[ENTITY_CURSOR].position.v[1] + 1.0f,
                            entities[ENTITY_CURSOR].position.v[2]});
                t3d_mat4_to_fixed(entities[ENTITY_DEFLECT_RING].matrix, &deflect_mat);
                //increase alpha for fade-in effect
                uint8_t alpha = (uint8_t)(200.0f * (game.deflect_timer / DEFLECT_DURATION));
                if (alpha > 200) alpha = 200;
                entities[ENTITY_DEFLECT_RING].color = RGBA32(0, 150, 255, alpha);

                draw_entity(&entities[ENTITY_DEFLECT_RING]);

            }
            continue;
        }

        if (entity_skip_culling[i]) {
            draw_entity(&entities[i]);
        } else if (is_entity_in_frustum(&entities[i], camera.position, camera.target, CAM_DEFAULT_FOV)) {
            draw_entity(&entities[i]);
        } else {
            culled_count++;
        }
    }

    // Draw asteroids and resources
    draw_entities_sorted(asteroids, ASTEROID_COUNT, NULL, asteroid_visible);
    draw_entities_sorted(resources, RESOURCE_COUNT, NULL, resource_visible);

    // Draw station with fade
    if (is_entity_in_frustum(&entities[ENTITY_STATION], camera.position, camera.target, CAM_DEFAULT_FOV)) {
        draw_entity_with_fade(&entities[ENTITY_STATION], 300.0f);
    } else {
        culled_count++;
    }


    // rdpq_mode_zbuf(true, false);
    draw_entity(&entities[ENTITY_GRID]);
    // rdpq_mode_zbuf(true, true);

    // Render particles at ~30Hz
    game.particle_render_timer += delta_time;
    if (game.particle_render_timer >= 0.033f) {
        draw_particles(viewport);
        game.particle_render_timer = 0.0f;
    }
    game.frame_count++;

    if (game.show_fps) {
        draw_fps_display(fps_stats.current, fps_stats.avg, fps_stats.min, fps_stats.max, debug_particle_count);
    }

    draw_info_bars();


    // Draw death timer if active
    if (game.death_timer_active) {
        int center_x = display_get_width() / 2;
        float time_remaining = 10.0f - game.death_timer;
        if (time_remaining < 0.0f) time_remaining = 0.0f;

        int seconds = (int)time_remaining;
        int tenths = (int)((time_remaining - seconds) * 10);

        rdpq_text_printf(NULL, FONT_CUSTOM, center_x - 20, 15, "%d.%d", seconds, tenths);
    }

    if (game.render_debug) {
        render_debug_ui(game.cursor_position, entities, resources, RESOURCE_COUNT, culled_count,
                        game.cursor_resource_val, game.drone_resource_val);
    }

    if (game.state == STATE_PAUSED || game.game_over) {
        draw_pause_menu();

    }

    // Draw countdown overlay
    if (game.state == STATE_COUNTDOWN) {
        draw_countdown();
    }

    rdpq_detach_show();
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main(void) {
    init_subsystems();
    T3DViewport viewport = t3d_viewport_create();

    // Load sprites
    sprite_t *background = sprite_load("rom:/bg1024v2.sprite");
    station_icon = sprite_load("rom:/station2.sprite");
    drill_icon = sprite_load("rom:/drill2.sprite");
    tile_icon = sprite_load("rom:/tile3.sprite");
    drone_icon = sprite_load("rom:/drone.sprite");
    drone_full_icon = sprite_load("rom:/drone_full.sprite");
    health_icon = sprite_load("rom:/health.sprite");

    // Create entities
    entities[ENTITY_STATION] = create_entity("rom:/stationew.t3dm", (T3DVec3){{0, DEFAULT_HEIGHT, 0}},
                                              0.75f, COLOR_STATION, DRAW_TEXTURED_LIT, 30.0f);
    entities[ENTITY_STATION].value = STATION_MAX_HEALTH;

    entities[ENTITY_CURSOR] = create_entity("rom:/cursor2.t3dm", game.cursor_position,
                                             0.562605f, COLOR_CURSOR, DRAW_SHADED, 10.0f);
    entities[ENTITY_CURSOR].value = CURSOR_MAX_HEALTH;

    entities[ENTITY_DRONE] = create_entity("rom:/dronenew.t3dm", (T3DVec3){{20.0f, DEFAULT_HEIGHT, 29.0f}},
                                            0.55f, COLOR_DRONE, DRAW_SHADED, 30.0f);
    entities[ENTITY_TILE] = create_entity("rom:/tile2.t3dm", (T3DVec3){{0, 1000, 0}},
                                           1.0f, COLOR_TILE, DRAW_SHADED, 10.0f);

    entities[ENTITY_DEFLECT_RING] = create_entity("rom:/tile2.t3dm", (T3DVec3){{0, 1000, 0}},
                                       1.0f, RGBA32(0, 150, 255, 200), DRAW_SHADED, 0.0f);
    entities[ENTITY_GRID] = create_entity("rom:/grid.t3dm", (T3DVec3){{0, 1, 0}},
                                           1.0f, COLOR_MAP, DRAW_SHADED, 0.0f);

    init_asteroids(asteroids, ASTEROID_COUNT);
    init_resources(resources, RESOURCE_COUNT);

    cursor_entity = &entities[ENTITY_CURSOR];


    //maybe items
    init_ambient_particles();

    // Load audio
    wav64_open(&sfx_mining, "rom:/ploop.wav64");
    wav64_open(&sfx_dcom, "rom:/dronecommand.wav64");
    wav64_open(&sfx_dfull, "rom:/dronecommand.wav64");
    wav64_open(&sfx_shiphit, "rom:/shiphit.wav64");

    if (game.bgm_track == 1) {
        play_bgm("rom:/nebrun.wav64");
    }

    float last_time = get_time_s() - (1.0f / 60.0f);

    // =============================================================================
    // Main Game Loop
    // =============================================================================

    for (;;) {
        float current_time = get_time_s();
        float delta_time = current_time - last_time;
        // last_time = current_time;

        joypad_poll();
        update_input();
        process_system_input(&viewport);

        // Handle countdown state
        if (game.state == STATE_COUNTDOWN) {
            game.countdown_timer -= delta_time;

            // Update camera during countdown so scene is visible
            update_camera(&viewport, game.cam_yaw, delta_time, game.cursor_position, game.fps_mode, cursor_entity);

            // Transition to playing when countdown finishes
            if (game.countdown_timer <= -0.5f) {
                game.state = STATE_PLAYING;
                game.game_over = false;
            }
        }

        if (game.state == STATE_PLAYING && !game.game_over) {
            update_cursor_movement(delta_time, cursor_entity);

            update_cursor_scale_by_distance(&entities[ENTITY_CURSOR], &entities[ENTITY_STATION], delta_time);

            process_game_input(delta_time);

             // Check deflect input every frame (before 30Hz collision check)
            // check_deflect_input();

            update_camera(&viewport, game.cam_yaw, delta_time, game.cursor_position, game.fps_mode, cursor_entity);
            update_screen_shake(delta_time);
            update_tile_visibility(&entities[ENTITY_TILE]);

            // Tile collision (only if drone not full)
            if (!game.drone_full) {
                check_tile_resource_collision(&entities[ENTITY_TILE], resources, RESOURCE_COUNT);
            }

            check_tile_following_status(&entities[ENTITY_DRONE]);

            // Tile scale animation
            if (game.drone_moving_to_station) {
                game.tile_scale_multiplier += delta_time * 3.0f;
                if (game.tile_scale_multiplier >= 4.0f) {
                    game.tile_scale_multiplier = 1.0f;
                }
            } else if (game.move_drone || game.tile_following_resource >= 0) {
                game.tile_scale_multiplier += delta_time * 1.25f;
                if (game.tile_scale_multiplier >= 1.0f) {
                    game.tile_scale_multiplier = 0.25f;
                }
            } else {
                game.tile_scale_multiplier = 0.25f;
            }

            // Drone movement
            if (game.move_drone) {
                move_drone_to_target(&entities[ENTITY_DRONE], delta_time, false);
            }

            // Update world
            update_asteroids(asteroids, ASTEROID_COUNT, delta_time);
            update_resources(resources, RESOURCE_COUNT, delta_time);
            update_particles(delta_time);

            // Update matrices
            update_entity_matrices(entities, ENTITY_COUNT);

            // Compute visibility
            compute_visibility(asteroids, asteroid_visible, ASTEROID_COUNT);
            compute_visibility(resources, resource_visible, RESOURCE_COUNT);

            // Asteroid matrices at ~30Hz
            game.asteroid_matrix_timer += delta_time;
            if (game.asteroid_matrix_timer >= 0.033f) {
                for (int i = 0; i < ASTEROID_COUNT; i++) {
                    if (asteroid_visible[i]) {
                        update_entity_matrix(&asteroids[i]);
                    }
                }
                game.asteroid_matrix_timer = 0.0f;
            }

            // Resource matrices
            for (int i = 0; i < RESOURCE_COUNT; i++) {
                if (resource_visible[i]) {
                    update_entity_matrix(&resources[i]);
                }
            }

            // Reset colors
            reset_resource_colors(resources, RESOURCE_COUNT);

            // Collision detection
            check_cursor_resource_collisions(&entities[ENTITY_CURSOR], resources, RESOURCE_COUNT, delta_time);
            check_cursor_station_collision(&entities[ENTITY_CURSOR], &entities[ENTITY_STATION]);
            check_drone_resource_collisions(&entities[ENTITY_DRONE], resources, RESOURCE_COUNT, delta_time);
            check_drone_station_collisions(&entities[ENTITY_DRONE], &entities[ENTITY_STATION], 1);

            if (game.drone_heal) {
                check_drone_cursor_collisions(&entities[ENTITY_DRONE], &entities[ENTITY_CURSOR], 1);
            }

            if (cursor_entity->value > 0 && game.ship_fuel > 0) {
                game.disabled_controls = false;
            } else  {
                game.disabled_controls = true;
            }

            // Asteroid collisions at ~30Hz
            game.collision_timer += delta_time;
            if (game.collision_timer >= 0.033f) {
                check_cursor_asteroid_deflection(&entities[ENTITY_CURSOR], asteroids, ASTEROID_COUNT);
                check_cursor_asteroid_collisions(&entities[ENTITY_CURSOR], asteroids, ASTEROID_COUNT, asteroid_visible);
                game.collision_timer = 0.0f;
            }

            // Update deflection timer
            update_deflect_timer(delta_time);

            // Update death timer (when cursor health = 0)
            if (entities[ENTITY_CURSOR].value <= 0 || game.ship_fuel <= 0) {
                if (!game.death_timer_active) {
                    game.death_timer_active = true;
                    game.death_timer = 0.0f;
                }

                game.death_timer += delta_time;
                if (game.death_timer >= 10.0f) {
                    // 10 seconds reached - show game over menu
                    game.game_over = true;
                    game.game_over_pause = true;
                    game.death_timer = 10.0f;
                    // Don't set reset here - let the menu handle it
                }
            } else {
                // Reset if health restored
                game.death_timer_active = false;
                game.death_timer = 0.0f;
            }

            update_color_flashes(delta_time);
            update_fps_stats(delta_time);
            // if (game.ship_acceleration) {
            //     update_ship_fuel(delta_time);
            // }
            update_ship_fuel(delta_time);
            update_difficulty(delta_time);



            game.blink_timer++;
            if (game.blink_timer > 20) game.blink_timer = 0;
        }

        // Handle reset (can happen from pause menu even when game_over)
        if (game.reset) {
            // Subtract a life on game over
            game.player_lives--;

            if (game.player_lives <= 0) {
                // Full reset - out of lives
                game.player_lives = 3;
                game.game_time = 0.0f;
                game.difficulty_multiplier = 1.0f;
                game.accumulated_credits = 0;

                // Start countdown
                game.state = STATE_COUNTDOWN;
                game.countdown_timer = 3.0f;
            } else {
                // Still have lives, go back to playing
                game.state = STATE_PLAYING;
            }

            entities[ENTITY_CURSOR].value = CURSOR_MAX_HEALTH;
            game.cursor_resource_val = 0;
            game.drone_resource_val = 0;
            game.drone_full = false;
            game.move_drone = false;
            game.drone_mining_resource = -1;
            game.tile_following_resource = -1;
            game.deflect_active = false;
            game.disabled_controls = false;
            game.ship_fuel = CURSOR_MAX_FUEL;
            game.death_timer_active = false;
            game.death_timer = 0.0f;
            game.game_over = false;
            game.game_over_pause = false;
            game.reset = false;
        }

        render_frame(&viewport, background, game.cam_yaw, delta_time);
        update_audio();
        rspq_wait();

        // Update last_time AFTER vsync/wait
        last_time = current_time;

    }

    // Cleanup

    cleanup_particles();
    sprite_free(background);
    sprite_free(station_icon);
    sprite_free(drill_icon);
    sprite_free(tile_icon);
    sprite_free(drone_icon);
    sprite_free(drone_full_icon);

    wav64_close(&sfx_mining);
    wav64_close(&sfx_dcom);
    wav64_close(&sfx_dfull);
    wav64_close(&sfx_shiphit);

    stop_bgm();
    free_all_entities(entities, ENTITY_COUNT);
    free_all_entities(asteroids, ASTEROID_COUNT);
    free_all_entities(resources, RESOURCE_COUNT);
    t3d_destroy();


    return 0;
}