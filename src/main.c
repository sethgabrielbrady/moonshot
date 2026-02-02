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
static Asteroid asteroids[ASTEROID_COUNT];  // Optimized asteroid struct
static Entity resources[RESOURCE_COUNT];
static Entity *cursor_entity = NULL;
static Entity *jets_entity = NULL;

// Visibility arrays for culling
static bool asteroid_visible[ASTEROID_COUNT];
static bool resource_visible[RESOURCE_COUNT];

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

// Asteroid-specific visibility with distance culling (uses optimized Asteroid struct)
static float asteroid_distance_sq[ASTEROID_COUNT];

static bool is_asteroid_in_frustum(Asteroid *asteroid, T3DVec3 cam_position, T3DVec3 cam_target, float fov_degrees) {
    float dx = asteroid->position.v[0] - cam_position.v[0];
    float dy = asteroid->position.v[1] - cam_position.v[1];
    float dz = asteroid->position.v[2] - cam_position.v[2];

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
    float radius = asteroid->scale * 50.0f;
    if (distance - radius > CAM_FAR_PLANE) {
        return false;
    }

    return true;
}

static void compute_asteroid_visibility(Asteroid *asteroids, bool *visibility, int count) {
    for (int i = 0; i < count; i++) {
        // Calculate distance from CAMERA (not cursor) for proper culling
        float dx = asteroids[i].position.v[0] - camera.position.v[0];
        float dz = asteroids[i].position.v[2] - camera.position.v[2];
        float dist_sq = dx * dx + dz * dz;
        asteroid_distance_sq[i] = dist_sq;

        // Too far from camera - don't draw at all
        if (dist_sq > ASTEROID_DRAW_DISTANCE_SQ) {
            visibility[i] = false;
        } else {
            // Within range - check frustum
            visibility[i] = is_asteroid_in_frustum(&asteroids[i], camera.position, camera.target, CAM_DEFAULT_FOV);
        }
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

    // In heal mode, don't stop moving - let collision detection handle arrival
    if (!game.drone_heal && distance_sq < DRONE_ARRIVE_THRESHOLD * DRONE_ARRIVE_THRESHOLD) {
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

    // Hide tile if drone is actively mining
    if (game.drone_mining_resource >= 0) {
        tile->position.v[0] = 0.0f;
        tile->position.v[1] = 1000.0f;
        tile->position.v[2] = 0.0f;
        return;
    }

    if (game.drone_heal) {
        // Follow cursor when in drone heal mode
        tile->position.v[0] = game.cursor_position.v[0];
        tile->position.v[1] = 8.0f;
        tile->position.v[2] = game.cursor_position.v[2];
    } else if (game.tile_following_resource >= 0 && game.tile_following_resource < RESOURCE_COUNT) {
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

    // Set FPS limit based on TV type (no need to change resolution - VI handles scaling)
    if (game.is_pal_system) {
        display_set_fps_limit(25);
        debugf("PAL system detected - 320x240 @ 25fps\n");
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
    custom_font = rdpq_font_load("rom:/striker.font64");
    icon_font = rdpq_font_load("rom:/spacerangertitleital.font64");

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
    // uint8_t color_ambient[4] = {0x60, 0x60, 0x60, 0xFF};
    uint8_t color_ambient[4] = {0x20, 0x20, 0x20, 0xFF};
    uint8_t color_directional[4] = {0xFF, 0xFF, 0xFF, 0xFF};
    T3DVec3 light_dir = {{100.0f, 200.0f, 0.0f}};

    t3d_vec3_norm(&light_dir);
    t3d_light_set_ambient(color_ambient);
    t3d_light_set_directional(0, color_directional, &light_dir);

    // Point light at station
    uint8_t station_light_color[4] = {0xFF, 0xCC, 0x88, 0xFF};  // Warm white/orange
    T3DVec3 station_light_pos = {{0.0f, 10.0f, 0.0f}};
    t3d_light_set_point(1, station_light_color, &station_light_pos, 100.0f, false);

    // Point light for wall (initial position, updated each frame)
    uint8_t wall_light_color[4] = {0xFF, 0xCC, 0x88, 0xFF};  // 255, 237, 41, 175
    T3DVec3 wall_light_pos = {{ 0.0f, WALL_HEIGHT, PLAY_AREA_RADIUS}};
    t3d_light_set_point(2, wall_light_color, &wall_light_pos, 100.0f, false);
}

static void update_wall_light(T3DVec3 wall_pos) {
    uint8_t wall_light_color[4] = {0xFF, 0xCC, 0x88,0xFF};  // 255, 237, 41, 175
    T3DVec3 light_pos = {{wall_pos.v[0], wall_pos.v[1], wall_pos.v[2]}};
    t3d_light_set_point(2, wall_light_color, &light_pos, 100.0f, true);
}

// =============================================================================
// Boundary Wall (Circular)
// =============================================================================

static float wall_orbit_angle = 0.0f;

static void update_boundary_wall(Entity *wall, T3DVec3 cursor_pos, float delta_time) {
    // Calculate distance from center
    float dist_sq = cursor_pos.v[0] * cursor_pos.v[0] + cursor_pos.v[2] * cursor_pos.v[2];
    float dist = sqrtf(dist_sq);

    // Distance from the edge of the play area
    float distance_to_edge = PLAY_AREA_RADIUS - dist;

    // Check if cursor is near edge
    bool near_edge = (distance_to_edge < WALL_FADE_START);

    float target_angle;

    if (near_edge) {
        // Snap to cursor direction
        target_angle = atan2f(cursor_pos.v[2], cursor_pos.v[0]);
        wall_orbit_angle = target_angle;

    } else {
        // Orbit around perimeter
        wall_orbit_angle += delta_time * 2.5f;  // Orbit speed
        if (wall_orbit_angle > TWO_PI) wall_orbit_angle -= TWO_PI;
        target_angle = wall_orbit_angle;
    }

    // Position wall at the edge
    wall->position.v[0] = cosf(target_angle) * PLAY_AREA_RADIUS;
    wall->position.v[1] = WALL_HEIGHT;
    wall->position.v[2] = sinf(target_angle) * PLAY_AREA_RADIUS;

    // Rotate wall to face inward (toward center)
    wall->rotation.v[1] = target_angle + T3D_PI / 2.0f;

    // Update wall point light position
    wall->color = RGBA32(255, 237, 41, 175);// 255, 237, 41, 175
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

    if (!cam_yaw) {
        rdpq_sprite_blit(background, BG_WIDTH, 0, NULL);
    } else if (needs_wrap) {
        rdpq_sprite_blit(background, -offset_x + BG_WIDTH, 0, NULL);
    }
}

// =============================================================================
// UI Drawing
// =============================================================================

// =============================================================================
// Gauge Drawing Helpers
// =============================================================================

// Build a string of slash marks based on percentage (2 slashes per 10%)
static void build_gauge_slashes(char *buffer, float percent) {
    // Clamp percent
    if (percent > 1.0f) percent = 1.0f;
    if (percent < 0.0f) percent = 0.0f;

    // Calculate slash count: 2 slashes per 10%, round up for partial
    // 1-5% = 1, 6-10% = 2, 11-15% = 3, etc.
    int slash_count = 0;
    if (percent > 0.0f) {
        slash_count = (int)((percent * 100.0f + 4.99f) / 5.0f);  // Round up to nearest 5%, then 1 slash per 5%
        if (slash_count > 20) slash_count = 20;
        if (slash_count < 1 && percent > 0.0f) slash_count = 1;
    }

    // Fill buffer with slashes
    for (int i = 0; i < slash_count; i++) {
        buffer[i] = '/';
    }
    buffer[slash_count] = '\0';
}

static void draw_cursor_fuel_bar(void) {
    float fuel_percent = game.ship_fuel / CURSOR_MAX_FUEL;
    if (fuel_percent > 1.0f) fuel_percent = 1.0f;
    if (fuel_percent < 0.0f) fuel_percent = 0.0f;

    if (fuel_percent <= 0.0f) return;

    // Build slash string based on fuel level
    char slashes[32];
    build_gauge_slashes(slashes, fuel_percent);

    int x = 23;  // Offset from health bar on X axis (moved 15px left)
    int y = SCREEN_HEIGHT - 49;

    rdpq_sync_pipe();
    rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
    rdpq_font_style(custom_font, 0, &(rdpq_fontstyle_t){.color = COLOR_FUEL_BAR});

    // Draw slashes twice offset by 1 pixel for thickness
    rdpq_text_printf(
        &(rdpq_textparms_t){
            .width = 280, .height = 40,
            .align = ALIGN_LEFT, .valign = VALIGN_BOTTOM,
            .char_spacing = 0
        },
        FONT_CUSTOM, x, y, "%s", slashes);
    rdpq_text_printf(
        &(rdpq_textparms_t){
            .width = 280, .height = 40,
            .align = ALIGN_LEFT, .valign = VALIGN_BOTTOM,
            .char_spacing = 0
        },
        FONT_CUSTOM, x + 1, y, "%s", slashes);
}

static void draw_entity_health_bar(Entity *entity, float max_value, int y_offset, int is_cursor) {
    if (entity->value < 0) entity->value = 0;

    float health_percent = entity->value / max_value;
    if (health_percent > 1.0f) health_percent = 1.0f;
    if (health_percent < 0.0f) health_percent = 0.0f;

    if (health_percent <= 0.0f) return;

    // Build slash string based on health level
    char slashes[32];
    build_gauge_slashes(slashes, health_percent);

    int x = 20;  // Moved 15px left
    int y = is_cursor ? (SCREEN_HEIGHT - 51) : (10 + y_offset - 25);

    rdpq_sync_pipe();
    rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
    rdpq_font_style(custom_font, 0, &(rdpq_fontstyle_t){.color = COLOR_HEALTH});

    // Draw slashes twice offset by 1 pixel for thickness
    rdpq_text_printf(
        &(rdpq_textparms_t){
            .width = 280, .height = 40,
            .align = ALIGN_LEFT, .valign = VALIGN_BOTTOM,
            .char_spacing = 0
        },
        FONT_CUSTOM, x, y, "%s", slashes);
    rdpq_text_printf(
        &(rdpq_textparms_t){
            .width = 280, .height = 40,
            .align = ALIGN_LEFT, .valign = VALIGN_BOTTOM,
            .char_spacing = 0
        },
        FONT_CUSTOM, x + 1, y, "%s", slashes);
}


static void draw_entity_resource_bar(int resource_val, float max_value, int y_offset, const char *label, bool show_triangle) {
    int bar_width = 40;
    int bar_height = 3;

    int x = display_get_width() - bar_width - 2;
    int y = SCREEN_HEIGHT - 40;

    float resource_percent = resource_val / max_value;
    if (resource_percent > 1.0f) resource_percent = 1.0f;
    if (resource_percent < 0.0f) resource_percent = 0.0f;

    int fill_width = (int)(bar_width * resource_percent);
    color_t bar_color = RGBA32(0, 191, 255, 255);

    rdpq_sync_pipe();
    rdpq_mode_combiner(RDPQ_COMBINER_FLAT);

    if (!show_triangle) {
        rdpq_font_style(custom_font, 0, &(rdpq_fontstyle_t){.color = COLOR_RESOURCE});
            rdpq_text_printf(
                &(rdpq_textparms_t){
                    .width = 280,
                    .height = 40,
                    .align = ALIGN_LEFT,
                    .valign = VALIGN_BOTTOM,
                    .char_spacing = 1
                },
                FONT_CUSTOM,
                20 ,  // X: moved 15px left
                y - 24,
                 "%d%s", (int)(resource_percent * 100), "%" );
    }

    int icon_x = x - 20;
    int icon_y = y - 6;

    if (show_triangle) {
        // Position drone icons 6px to right of resource percentage (which is at x=20)
        // Resource text is roughly 30-40px wide, so place icons at ~60px from left
        icon_x = 60;  // 6px right of resource percentage text
        int action_x = icon_x;
        int action_y = icon_y + 6;

        rdpq_sync_pipe();
        rdpq_set_mode_standard();
        rdpq_mode_combiner(RDPQ_COMBINER1((TEX0, 0, PRIM, 0), (TEX0, 0, PRIM, 0)));

        if (game.drone_resource_val >= DRONE_MAX_RESOURCES) {
            rdpq_set_prim_color(RGBA32(220, 0, 115, 255));
            game.drone_collecting_resource = false;
        } else {
            rdpq_set_prim_color(RGBA32(0, 255, 191, 255));
        }


        if (game.blink_timer < 10) {
            rdpq_set_prim_color(COLOR_FLAME);

            if (game.drone_collecting_resource) {
                rdpq_sync_pipe();
                rdpq_set_mode_standard();
                rdpq_mode_combiner(RDPQ_COMBINER1((TEX0, 0, PRIM, 0), (TEX0, 0, PRIM, 0)));
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
                // rdpq_set_prim_color(COLOR_RESOURCE);
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
                rdpq_set_prim_color(COLOR_HEALTH);
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
                rdpq_set_prim_color(COLOR_FLAME); //
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
    // Safety check for invalid timer values
    if (!isfinite(game.countdown_timer) || game.countdown_timer < -10.0f || game.countdown_timer > 100.0f) {
        return;
    }

    int count = (int)game.countdown_timer + 1;

    if (count > 1 && count <= 4) {
        rdpq_font_style(icon_font, 0, &(rdpq_fontstyle_t){.color = COLOR_ASTEROID});
        rdpq_text_printf(
            &(rdpq_textparms_t){
                .width = 280,
                .height = 40,
                .align = ALIGN_CENTER,
                .valign = VALIGN_CENTER,
                .char_spacing = 1
            },
            FONT_ICON,
            display_get_width() / 2 - 140,
            display_get_height() / 2 - 20,
                "%d", count - 1 );

    } else if (game.countdown_timer < 2) {
        rdpq_font_style(icon_font, 0, &(rdpq_fontstyle_t){.color = COLOR_HEALTH});
        rdpq_text_printf(
            &(rdpq_textparms_t){
                .width = 280,
                .height = 40,
                .align = ALIGN_CENTER,
                .valign = VALIGN_CENTER,
                .char_spacing = 1
            },
            FONT_ICON,
            display_get_width() / 2 - 140,
            display_get_height() / 2 - 20,
                "GO!" );

    } else {
         rdpq_font_style(icon_font, 0, &(rdpq_fontstyle_t){.color = RGBA32(255,255,255, 255)});
    }
}

static void draw_game_timer(void) {
    // Convert game_time (seconds) to MM:SS format
    int total_seconds = (int)game.game_time;
    int minutes = total_seconds / 60;
    int seconds = total_seconds % 60;
    int hundredths = (int)((game.game_time - total_seconds) * 100);

    // Position in bottom right corner
    int x = 38;
    int y = 20;


    // Draw timer
    rdpq_sync_pipe();
    rdpq_font_style(custom_font, 0, &(rdpq_fontstyle_t){.color = COLOR_ASTEROID});

    rdpq_text_printf(
        &(rdpq_textparms_t){.char_spacing = 1},
        FONT_CUSTOM,
        display_get_width() - 64,
        display_get_height() - 15,
        "%d:%02d.%02d", minutes, seconds, hundredths);


    rdpq_font_style(custom_font, 0, &(rdpq_fontstyle_t){.color = COLOR_HEALTH});
    rdpq_text_printf(
        &(rdpq_textparms_t){.char_spacing = 1},
        FONT_CUSTOM,
        display_get_width() - 64,
        display_get_height() - 25,
        "c: %d", game.accumulated_credits);

    float fuel_percent = game.ship_fuel / CURSOR_MAX_FUEL;
    float health_percent = cursor_entity->value / CURSOR_MAX_HEALTH;

    // Show status message if timer active (other messages like "Credits +X")
    if (game.status_message_timer > 0.0f) {
        rdpq_sync_pipe();
        rdpq_font_style(custom_font, 0, &(rdpq_fontstyle_t){.color = COLOR_ASTEROID });
        rdpq_text_printf(
            &(rdpq_textparms_t){
                .width = 280,
                .height = 40,
                .align = ALIGN_CENTER,
                .valign = VALIGN_TOP,
                .char_spacing = 1
            },
            FONT_CUSTOM,
            display_get_width() / 2 - 140,
            10,
            "%s", game.status_message
        );
    } else {
        // Only show low fuel/health warnings when no other message is showing
        bool low_fuel = fuel_percent < 0.3f;
        bool low_health = health_percent < 0.3f;

        if (low_fuel || low_health) {
            bool show_fuel_warning = false;

            if (low_fuel && low_health) {
                // Both low - alternate between them
                show_fuel_warning = ((int)(game.blink_timer / 10) % 2 == 0);
            } else {
                show_fuel_warning = low_fuel;
            }

            if ((int)(game.blink_timer / 10) % 2 == 0) {
                rdpq_sync_pipe();
                if (show_fuel_warning) {
                    rdpq_font_style(custom_font, 0, &(rdpq_fontstyle_t){.color = COLOR_FUEL_BAR});
                    rdpq_text_printf(
                        &(rdpq_textparms_t){
                            .width = 280,
                            .height = 40,
                            .align = ALIGN_CENTER,
                            .valign = VALIGN_TOP,
                            .char_spacing = 1
                        },
                        FONT_CUSTOM,
                        display_get_width() / 2 - 140,
                        10,
                        "%s", "--LOW FUEL--"
                    );
                } else {
                    rdpq_font_style(custom_font, 0, &(rdpq_fontstyle_t){.color = COLOR_HEALTH});
                    rdpq_text_printf(
                        &(rdpq_textparms_t){
                            .width = 280,
                            .height = 40,
                            .align = ALIGN_CENTER,
                            .valign = VALIGN_TOP,
                            .char_spacing = 1
                        },
                        FONT_CUSTOM,
                        display_get_width() / 2 - 140,
                        10,
                        "%s", "--NEED REPAIR--"
                    );
                }
            }
        }
    }

    rdpq_font_style(custom_font, 0, &(rdpq_fontstyle_t){.color = RGBA32(255, 255, 255, 255)});
}


static void draw_info_bars(void) {
    // Reset render mode to ensure clean state for UI
    rdpq_set_mode_standard();
    draw_cursor_fuel_bar();
    draw_entity_health_bar(cursor_entity, CURSOR_MAX_HEALTH, 20, 1);
    draw_entity_resource_bar(game.cursor_resource_val, CURSOR_RESOURCE_CAPACITY, 25, "Procyon", false);
    draw_entity_resource_bar(game.drone_resource_val, DRONE_MAX_RESOURCES, 225, "PUP", true);
    draw_game_timer();
}


// update station value to drop resources over time
static float fuel_drain_accumulated = 0.0f;
static void update_ship_fuel(float delta_time, bool accelerating) {
    const float FUEL_DRAIN_RATE = 1.5f; // resources per second
    if (accelerating) {
        fuel_drain_accumulated += FUEL_DRAIN_RATE * 2.0f * delta_time;
        spawn_ship_trail(entities[ENTITY_CURSOR].position, game.cursor_velocity, COLOR_FUEL_BAR);
    } else {
        fuel_drain_accumulated += FUEL_DRAIN_RATE * 1.25f * delta_time;
    }

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
    update_wall_light(entities[ENTITY_WALL].position);


    int light_count = 3;  // 1 directional + 2 point lights (station + wall)
    t3d_light_set_count(light_count);

    bool entity_skip_culling[ENTITY_COUNT] = {false};
    entity_skip_culling[ENTITY_STATION_V] = true;
    entity_skip_culling[ENTITY_CURSOR] = true;
    entity_skip_culling[ENTITY_STATION] = true;

    // Draw main entities
    for (int i = 0; i < ENTITY_COUNT; i++) {
        if (i == ENTITY_STATION || i == ENTITY_STATION_V) continue;

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
                entities[ENTITY_DEFLECT_RING].color = RGBA32(138, 0, 196, alpha);

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

    // Draw asteroids (optimized with matrix pool) and resources - skip during countdown
    if (game.state != STATE_COUNTDOWN) {
        draw_asteroids_optimized(asteroids, asteroid_visible, asteroid_distance_sq, ASTEROID_COUNT);
    }
    draw_entities_sorted(resources, RESOURCE_COUNT, NULL, resource_visible);

    // Draw station - disable Z-write to prevent self Z-fighting
    rdpq_mode_zbuf(true, false);  // Z-read on, Z-write off
    draw_entity(&entities[ENTITY_STATION]);
    rdpq_mode_zbuf(true, true);   // Restore Z-write
    rdpq_sync_pipe();

    rdpq_mode_zbuf(true, false);  // Z-read on, Z-write off
    draw_entity(&entities[ENTITY_STATION_V]);
    rdpq_mode_zbuf(true, true);   // Restore Z-write
    rdpq_sync_pipe();

    // Render particles at ~30Hz
    game.particle_render_timer += delta_time;
    if (game.particle_render_timer >= 0.033f) {
        draw_particles(viewport);
        game.particle_render_timer = 0.0f;
    }
    game.frame_count++;

    if (game.show_fps) {
        draw_fps_display(fps_stats.current, fps_stats.avg, fps_stats.min, fps_stats.max);
    }

    draw_info_bars();


    // Track was_dying state and GO display using integer frame counter
    static bool was_dying = false;
    static int go_display_frames = 0;

    // Update GO display frame counter
    if (go_display_frames > 0) {
        go_display_frames--;
        if (go_display_frames == 0) {
            was_dying = false;
        }
    }

    if (game.death_timer_active) {
        was_dying = true;
        go_display_frames = 0;
    }
    if (was_dying && !game.death_timer_active && go_display_frames == 0) {
        go_display_frames = 30;  // About 1 second at 30fps
        was_dying = false;
    }
    if (game.death_timer_active) {
        float time_remaining = 10.0f - game.death_timer;
        if (time_remaining < 0.0f) time_remaining = 0.0f;

        int seconds = (int)time_remaining;
        int tenths = (int)((time_remaining - seconds) * 10);

        rdpq_font_style(icon_font, 0, &(rdpq_fontstyle_t){.color = COLOR_WARNING});
        rdpq_text_printf(
            &(rdpq_textparms_t){
                .width = display_get_width() - 60,                // Box width for alignment
                .height = 40,                // Box height for alignment
                .align = ALIGN_RIGHT,       // Horizontal alignment
                .valign = VALIGN_TOP,        // Vertical alignment
                .char_spacing = 1
            },
            FONT_ICON,
             28,  // X: right edge of box
             5,
               "%d", seconds );
    }

    if (go_display_frames > 0 && !game.death_timer_active) {
        int center_x = display_get_width() / 2;
        int center_y = SCREEN_HEIGHT / 2;
        rdpq_font_style(icon_font, 0, &(rdpq_fontstyle_t){.color = COLOR_HEALTH});
        rdpq_text_printf(
            &(rdpq_textparms_t){
                .width = 280,
                .height = 40,
                .align = ALIGN_CENTER,
                .valign = VALIGN_CENTER,
                .char_spacing = 1
            },
            FONT_ICON,
            display_get_width() / 2 - 140,
            display_get_height() / 2 - 20,
               "GO!");
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
    drone_full_icon = sprite_load("rom:/drone_full.sprite");
    health_icon = sprite_load("rom:/health.sprite");
    rumble_icon = sprite_load("rom:/rpak.sprite");


    // Create entities
    entities[ENTITY_STATION] = create_entity("rom:/stationring2.t3dm", (T3DVec3){{0, DEFAULT_HEIGHT, 0}},
                                              1.0f, COLOR_STATION, DRAW_TEXTURED_LIT, 30.0f);
    entities[ENTITY_STATION_V] = create_entity("rom:/ringvert.t3dm", (T3DVec3){{0, 1, 0}},
                                           1.0f, COLOR_MAP, DRAW_SHADED, 0.0f);

    entities[ENTITY_CURSOR] = create_entity("rom:/cursor3.t3dm", game.cursor_position,
                                             0.562605f, COLOR_CURSOR, DRAW_SHADED, 10.0f);
    entities[ENTITY_CURSOR].value = CURSOR_MAX_HEALTH;

    entities[ENTITY_JETS] = create_entity("rom:/jets.t3dm", game.cursor_position,
                                             0.562605f, RGBA32(138, 0, 196, 0), DRAW_SHADED, 10.0f);

    entities[ENTITY_DRONE] = create_entity("rom:/dronenew.t3dm", (T3DVec3){{60.0f, DEFAULT_HEIGHT, 69.0f}},
                                            0.55f, COLOR_DRONE, DRAW_SHADED, 30.0f);
    entities[ENTITY_TILE] = create_entity("rom:/tile2.t3dm", (T3DVec3){{0, 1000, 0}},
                                           1.0f, COLOR_TILE, DRAW_SHADED, 10.0f);
    entities[ENTITY_LOADER] = create_entity("rom:/loader2.t3dm", (T3DVec3){{0, 1, 0}},
                                           1.0f,  RGBA32(255, 237, 41, 175), DRAW_SHADED, 50.0f);

    entities[ENTITY_LOADER_VERT] = create_entity("rom:/loader_vert2.t3dm", (T3DVec3){{0, 1, 0}},
                                           1.0f,  RGBA32(255, 237, 41, 175), DRAW_SHADED, 50.0f);

    entities[ENTITY_DEFLECT_RING] = create_entity("rom:/sphere.t3dm", (T3DVec3){{0, 1000, 0}},
                                       1.0f, RGBA32(0, 150, 255, 200), DRAW_SHADED, 0.0f);

    entities[ENTITY_WALL] = create_entity("rom:/wall.t3dm", (T3DVec3){{0, 1000, 0}},
                                           1.0f, RGBA32(255, 237, 41, 175), DRAW_SHADED, 0.0f);

    init_asteroids_optimized(asteroids, ASTEROID_COUNT);
    init_resources(resources, RESOURCE_COUNT);

    cursor_entity = &entities[ENTITY_CURSOR];
    jets_entity = &entities[ENTITY_JETS];

    //maybe items
    init_ambient_particles();


    // Load audio
    wav64_open(&sfx_mining, "rom:/ploop.wav64");
    wav64_open(&sfx_dcom, "rom:/dronecommand.wav64");
    wav64_open(&sfx_dfull, "rom:/dronecommand.wav64");
    wav64_open(&sfx_shiphit, "rom:/shiphit.wav64");

    // Play title screen music
    play_bgm("rom:/lunramtit.wav64");

    // =============================================================================
    // Main Game Loop
    // =============================================================================

    for (;;) {
        float delta_time = display_get_delta_time();

        // Clamp delta_time to prevent issues
        if (delta_time < 0.001f) delta_time = 0.001f;
        if (delta_time > 0.1f) delta_time = 0.1f;

        joypad_poll();
        update_input();

        //defaul to white font color
        rdpq_font_style(custom_font, 0, &(rdpq_fontstyle_t){.color = RGBA32(255, 255, 255, 255)});

        // Handle title screen
        if (game.state == STATE_TITLE) {
            update_audio();
            static float title_cam_yaw = 0.0f;

            // B to quit
            if (input.pressed.b) {
                stop_bgm();
                // Exit the game
                break;
            }

            if (input.pressed.start) {
                stop_bgm();
                update_audio();

                game.state = STATE_COUNTDOWN;
                game.countdown_timer = 4.0f;

                // Start game music
                if (game.bgm_track == 1) {
                    play_bgm("rom:/nebrunv3.wav64");
                } else if (game.bgm_track == 2) {
                    play_bgm("rom:/coshouv2.wav64");
                } else if (game.bgm_track == 3) {
                    play_bgm("rom:/lunramtit.wav64");
                } else if (game.bgm_track == 4) {
                    // Random - pick 1, 2, or 3
                    int random_track = (rand() % 3) + 1;
                    if (random_track == 1) {
                        play_bgm("rom:/nebrunv3.wav64");
                    } else if (random_track == 2) {
                        play_bgm("rom:/coshouv2.wav64");
                    } else if (random_track == 3) {
                        play_bgm("rom:/lunramtit.wav64");
                    }
                }

                // Reset asteroids off-screen when starting game
                for (int i = 0; i < ASTEROID_COUNT; i++) {
                    reset_asteroid(&asteroids[i]);
                    // Move asteroids far away so they're not visible during countdown
                    asteroids[i].position.v[0] = 2000.0f + (i * 10.0f);
                    asteroids[i].position.v[2] = 2000.0f + (i * 10.0f);
                }
                // Reset camera yaw
                game.cam_yaw = CAM_ANGLE_YAW;
                title_cam_yaw = 0.0f;
            }

            // Slowly pan the camera
            title_cam_yaw += delta_time * 5.0f;  // Slow rotation speed
            // Robust bounds check (handles NaN/overflow)
            if (title_cam_yaw >= 360.0f || title_cam_yaw < 0.0f || !isfinite(title_cam_yaw)) {
                title_cam_yaw = fmodf(title_cam_yaw, 360.0f);
                if (title_cam_yaw < 0.0f || !isfinite(title_cam_yaw)) title_cam_yaw = 0.0f;
            }

            // Update asteroids
            update_asteroids_optimized(asteroids, ASTEROID_COUNT, delta_time);

            // Rotate station for visual effect
            entities[ENTITY_STATION_V].rotation.v[0] += delta_time * 0.1f;
            if (entities[ENTITY_STATION_V].rotation.v[0] > TWO_PI) {
                entities[ENTITY_STATION_V].rotation.v[0] -= TWO_PI;
            }

            // Update camera for title screen (use title_cam_yaw for slow pan)
            update_camera(&viewport, title_cam_yaw, delta_time, (T3DVec3){{0, 0, 0}}, false, cursor_entity);

            // Perform visibility/culling for asteroids
            for (int i = 0; i < ASTEROID_COUNT; i++) {
                float dx = asteroids[i].position.v[0] - camera.position.v[0];
                float dz = asteroids[i].position.v[2] - camera.position.v[2];
                asteroid_distance_sq[i] = dx * dx + dz * dz;
                asteroid_visible[i] = (asteroid_distance_sq[i] < ASTEROID_DRAW_DISTANCE_SQ);
            }

            // Render 3D scene
            rdpq_attach(display_get(), display_get_zbuf());
            render_background(background, title_cam_yaw);

            t3d_frame_start();
            t3d_viewport_attach(&viewport);
            t3d_screen_clear_depth();
            setup_lighting();

            int light_count = 1;
            t3d_light_set_count(light_count);

            // Draw asteroids
            draw_asteroids_optimized(asteroids, asteroid_visible, asteroid_distance_sq, ASTEROID_COUNT);

            // Sync before drawing station
            rdpq_sync_pipe();

            // Draw station entities
            update_entity_matrix(&entities[ENTITY_STATION]);
            update_entity_matrix(&entities[ENTITY_STATION_V]);
            rdpq_mode_zbuf(true, false);  // Z-read on, Z-write off
            draw_entity(&entities[ENTITY_STATION]);
            rdpq_sync_pipe();
            draw_entity(&entities[ENTITY_STATION_V]);
            rdpq_mode_zbuf(true, true);   // Restore Z-write

            // Sync before switching to 2D text rendering
            rdpq_sync_pipe();

            // Draw title text on top
            rdpq_font_style(icon_font, 0, &(rdpq_fontstyle_t){.color = RGBA32(255, 255, 255, 255)});
            rdpq_text_printf(
                &(rdpq_textparms_t){
                    .width = display_get_width(),
                    .height = 40,
                    .align = ALIGN_CENTER,
                    .valign = VALIGN_TOP,
                    .char_spacing = 1
                },
                FONT_ICON,
                    2,
                    59,
                 "%s", "AsteRisk" );

            rdpq_font_style(icon_font, 0, &(rdpq_fontstyle_t){.color = COLOR_ASTEROID});
             rdpq_text_printf(
                &(rdpq_textparms_t){
                    .width = display_get_width(),
                    .height = 40,
                    .align = ALIGN_CENTER,
                    .valign = VALIGN_TOP,
                    .char_spacing = 1
                },
                FONT_ICON,
                    0,
                    60,
                 "%s", "AsteRisk" );

            rdpq_font_style(custom_font, 0, &(rdpq_fontstyle_t){.color = COLOR_HEALTH});
            rdpq_text_printf(
                &(rdpq_textparms_t){
                    .width = display_get_width(),
                    .height = 40,
                    .align = ALIGN_CENTER,
                    .valign = VALIGN_BOTTOM,
                    .char_spacing = 1
                },
                FONT_CUSTOM,
                display_get_width() / 2 - 160,
                SCREEN_HEIGHT / 2 + 10,
                 "%s", "Press Start" );

            // B to Quit option
            rdpq_font_style(custom_font, 0, &(rdpq_fontstyle_t){.color = RGBA32(180, 180, 180, 255)});
            rdpq_text_printf(
                &(rdpq_textparms_t){
                    .width = display_get_width(),
                    .height = 40,
                    .align = ALIGN_CENTER,
                    .valign = VALIGN_BOTTOM,
                    .char_spacing = 1
                },
                FONT_CUSTOM,
                display_get_width() / 2 - 160,
                SCREEN_HEIGHT / 2 + 25,
                 "%s", "B to Quit" );

            // Copyright notice bottom left
            rdpq_font_style(custom_font, 0, &(rdpq_fontstyle_t){.color = RGBA32(120, 120, 120, 255)});
            rdpq_text_printf(
                &(rdpq_textparms_t){
                    .char_spacing = 1
                },
                FONT_CUSTOM,
                10,
                display_get_height() - 15,
                 "%s", "2026 Brainpan" );

            //draw the rumble icon image - moved left by 15 pixels
            if (rumble_icon) {
                rdpq_set_prim_color(RGBA32(155, 44, 18, 155));
                rdpq_sprite_blit(rumble_icon, display_get_width() - 35 - (rumble_icon->width / 2),
                                 display_get_height() - rumble_icon->height - 10,
                                 &(rdpq_blitparms_t){
                                     .scale_x = 1.0f, .scale_y = 1.0f
                                 });
                rdpq_set_prim_color(RGBA32(255, 74, 28, 255));
                rdpq_sprite_blit(rumble_icon, display_get_width() - 36 - (rumble_icon->width / 2),
                                 display_get_height() - rumble_icon->height - 10,
                                 &(rdpq_blitparms_t){
                                     .scale_x = 1.0f, .scale_y = 1.0f
                                 });
            }


            rdpq_detach_show();
            continue;
        }

        process_system_input(&viewport);

        // Update message queue (handles timer and cycling through queued messages)
        update_message_queue(delta_time);

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
            update_cursor_movement(delta_time, cursor_entity, jets_entity);
            process_game_input(delta_time);
            update_camera(&viewport, game.cam_yaw, delta_time, game.cursor_position, game.fps_mode, cursor_entity);
            update_screen_shake(delta_time);
            update_tile_visibility(&entities[ENTITY_TILE]);
            update_boundary_wall(&entities[ENTITY_WALL], game.cursor_position, delta_time);

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
            if (game.drone_heal) {
                // Continuously update target to follow cursor in heal mode
                game.drone_target_position.v[0] = game.cursor_position.v[0];
                game.drone_target_position.v[1] = game.cursor_position.v[1];
                game.drone_target_position.v[2] = game.cursor_position.v[2];
                game.move_drone = true;
            }
            if (game.move_drone) {
                move_drone_to_target(&entities[ENTITY_DRONE], delta_time, false);
            }

            // Update world
            update_asteroids_optimized(asteroids, ASTEROID_COUNT, delta_time);
            update_resources(resources, RESOURCE_COUNT, delta_time);
            update_particles(delta_time);

            // Rotate station slowly
            entities[ENTITY_STATION].rotation.v[1] += delta_time * 0.1f;
            if (entities[ENTITY_STATION].rotation.v[1] > TWO_PI) {
                entities[ENTITY_STATION].rotation.v[1] -= TWO_PI;
            }
            entities[ENTITY_STATION_V].rotation.v[0] += delta_time * 0.1f;
            if (entities[ENTITY_STATION_V].rotation.v[0] > TWO_PI) {
                entities[ENTITY_STATION_V].rotation.v[0] -= TWO_PI;
            }






            float roation_speed = 0.9f;
            // game.hauled_resources, rotate the loader faster for 2 secons
            if (game.hauled_resources) {
                if (game.hauled_resources_timer < 1.0f) {
                    roation_speed = 0.9f + (2.7f * (game.hauled_resources_timer / 1.0f));
                } else {
                    roation_speed = 3.6f;
                }
                game.hauled_resources_timer += delta_time;
                if (game.hauled_resources_timer >= 4.0f) {
                    game.hauled_resources = false;
                    game.hauled_resources_timer = 0.0f;
                }

                // Spawn celebration particles around the loader
                spawn_loader_sparks(entities[ENTITY_LOADER].position);
            }
            entities[ENTITY_LOADER].rotation.v[1] -= delta_time * roation_speed;
            if (entities[ENTITY_LOADER].rotation.v[1] < 0.0f) {
                entities[ENTITY_LOADER].rotation.v[1] += TWO_PI;
            }

            entities[ENTITY_LOADER_VERT].rotation.v[0] += delta_time * roation_speed;
            if (entities[ENTITY_LOADER_VERT].rotation.v[0] < 0.0f) {
                entities[ENTITY_LOADER_VERT].rotation.v[0] += TWO_PI;
            }


            // Update matrices
            update_entity_matrices(entities, ENTITY_COUNT);

            // Compute visibility (asteroids use optimized distance-based culling)
            compute_asteroid_visibility(asteroids, asteroid_visible, ASTEROID_COUNT);
            compute_visibility(resources, resource_visible, RESOURCE_COUNT);

            // Note: Asteroid matrices are now handled in draw_asteroids_optimized()
            // using the matrix pool - no per-asteroid matrix updates needed here

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

            // Asteroid collisions at ~30Hz (using optimized functions)
            game.collision_timer += delta_time;
            if (game.collision_timer >= 0.033f) {
                check_cursor_asteroid_deflection_opt(&entities[ENTITY_CURSOR], asteroids, ASTEROID_COUNT);
                check_cursor_asteroid_collisions_opt(&entities[ENTITY_CURSOR], asteroids, ASTEROID_COUNT, asteroid_visible, delta_time);
                game.collision_timer = 0.0f;
            }

            check_loader_asteroid_collisions_opt(&entities[ENTITY_LOADER], asteroids, ASTEROID_COUNT, delta_time);

            // Update deflection timer
            update_deflect_timer(delta_time);

            // Update rumble timer
            update_rumble(delta_time);

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
                    game.menu_selection = 0;  // Reset menu selection for Continue/Quit
                    game.death_timer = 10.0f;
                    stop_rumble();
                    // Don't set reset here - let the menu handle it
                }
            } else {
                // Reset if health restored
                game.death_timer_active = false;
                game.death_timer = 0.0f;
            }


            update_color_flashes(delta_time);
            update_fps_stats(delta_time);
            update_ship_fuel(delta_time, game.ship_acceleration);
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
                for (int i = 0; i < ASTEROID_COUNT; i++) {
                    reset_asteroid(&asteroids[i]);
                }
                for (int i = 0; i < RESOURCE_COUNT; i++) {
                    reset_entity(&resources[i], RESOURCE);
                }
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
        update_audio();  // Extra call to prevent audio stutter at low framerates
    }

    // Cleanup
    cleanup_particles();
    sprite_free(background);
    sprite_free(station_icon);
    sprite_free(drill_icon);
    sprite_free(tile_icon);
    // sprite_free(drone_icon);
    sprite_free(drone_full_icon);

    wav64_close(&sfx_mining);
    wav64_close(&sfx_dcom);
    wav64_close(&sfx_dfull);
    wav64_close(&sfx_shiphit);

    stop_bgm();
    free_all_entities(entities, ENTITY_COUNT);
    free_shared_models();  // Frees asteroid model and matrix pool
    free_all_entities_shared(resources, RESOURCE_COUNT);  // Resources use shared model
    t3d_destroy();
    return 0;
}