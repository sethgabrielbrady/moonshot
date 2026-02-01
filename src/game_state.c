#include "game_state.h"
#include "constants.h"


// =============================================================================
// Global Game State Instance
// =============================================================================

GameStateData game = {
    // Core game state
    .state = STATE_TITLE,
    .game_over = false,
    .game_over_pause = false,

    // Menu
    .menu_selection = 0,

    // Display/rendering options
    .fps_mode = false,
    .render_debug = false,
    .hi_res_mode = false,
    .show_fps = false,
    .render_background_enabled = true,
    .show_controls = false,
    .show_tutorial = false,
    .show_credits = false,
    .fps_limit = 0,
    .is_pal_system = false,

    // Audio
    .bgm_track = 5,  // 5 = Random

    // Blink timer
    .blink_timer = 0,

    // Damage tracking
    .station_last_damage = 0,
    .cursor_last_damage = 0,

    // Resource values
    .resource_val = 0,
    .cursor_resource_val = 0,
    .drone_resource_val = 0,
    .highlighted_resource = -1,

    // Mining state
    .cursor_mining_accumulated = 0.0f,
    .drone_mining_accumulated = 0.0f,
    .cursor_mining_resource = -1,
    .drone_mining_resource = -1,
    .cursor_is_mining = false,
    .drone_is_mining = false,

    // Drone state
    .move_drone = false,
    .drone_heal = false,
    .drone_collecting_resource = false,
    .drone_moving_to_resource = false,
    .drone_moving_to_station = false,
    .drone_full = false,
    .drone_target_position = {{0.0f, 0.0f, 0.0f}},
    .drone_target_rotation = 0.0f,

    // Tile state
    .tile_following_resource = -1,
    .tile_scale_multiplier = 1.0f,

    // Cursor state
    .cursor_scale_multiplier = 1.0f,
    .cursor_position = {{200.0f, CURSOR_HEIGHT, 100.0f}},
    .cursor_velocity = {{0.0f, 0.0f, 0.0f}},

    // Camera
    .cam_yaw = CAM_ANGLE_YAW,

    // Iframe timers
    .cursor_iframe_timer = 0.0f,

    // Frame counting
    .frame_count = 0,

    // Time accumulators
    .particle_render_timer = 0.0f,
    // .ambient_particle_timer = 0.0f,
    .asteroid_matrix_timer = 0.0f,
    .collision_timer = 0.0f,

    // Fixed timestep
    .accumulator = 0.0f,
    .reset = false,
    // Deflection state
    .deflect_timer = 0.0f,
    .deflect_active = false,
    .deflect_count = 0,
    .disabled_controls = false,
    .player_lives = 3,

    // Death timer
    .death_timer = 0.0f,
    .death_timer_active = false,
    .go_display_timer = 0.0f,
    .accumulated_credits = 0,
    .ship_fuel = CURSOR_MAX_FUEL,
    .ship_acceleration = false,

    // Difficulty progression
    .game_time = 0.0f,
    .difficulty_multiplier = 1.0f,

    // Countdown
    .countdown_timer = 3.0f,
    .hauled_resources = false,
    .hauled_resources_timer = 0.0f,
    .status_message = "",
    .status_message_timer = 0.0f
};

// =============================================================================
// Font Instances
// =============================================================================

rdpq_font_t *custom_font = NULL;
rdpq_font_t *icon_font = NULL;

// =============================================================================
// Sprite/Icon Instances
// =============================================================================

sprite_t *station_icon = NULL;
sprite_t *tile_icon = NULL;
sprite_t *drill_icon = NULL;
sprite_t *drone_icon = NULL;
sprite_t *drone_full_icon = NULL;
sprite_t *health_icon = NULL;

// =============================================================================
// Functions
// =============================================================================

void init_game_state(void) {
    game.state = STATE_TITLE;
    game.countdown_timer = 4.0f;
    game.game_over = false;
    game.game_over_pause = false;
    game.cam_yaw = CAM_ANGLE_YAW;
    game.cursor_position = (T3DVec3){{200.0f, CURSOR_HEIGHT, 100.0f}};
    game.cursor_velocity = (T3DVec3){{0.0f, 0.0f, 0.0f}};
}


void pause_game(void) {
    game.state = STATE_PAUSED;
}

void unpause_game(void) {
    game.state = STATE_PLAYING;
}

void set_game_over(void) {
    game.state = STATE_GAME_OVER;
    game.game_over = true;
}

// =============================================================================
// Difficulty Progression
// =============================================================================

// Configuration - tweak these to adjust difficulty curve
#define DIFFICULTY_INTERVAL      60.0f   // Seconds between each speed increase
#define DIFFICULTY_INCREASE      0.25f   // 25% increase per interval
#define DIFFICULTY_MAX_MULTIPLIER 5.0f   // Cap to prevent insane speeds

void update_difficulty(float delta_time) {
    game.game_time += delta_time;

    // Calculate how many 60-second intervals have passed
    int intervals = (int)(game.game_time / DIFFICULTY_INTERVAL);

    // Each interval adds 25%: 1.0, 1.25, 1.5625, 1.953, etc. (compound)
    // Using compound: multiplier = (1 + 0.25)^intervals
    float multiplier = 1.0f;
    for (int i = 0; i < intervals; i++) {
        multiplier *= (1.0f + DIFFICULTY_INCREASE);
    }

    // Cap at maximum
    if (multiplier > DIFFICULTY_MAX_MULTIPLIER) {
        multiplier = DIFFICULTY_MAX_MULTIPLIER;
    }

    game.difficulty_multiplier = multiplier;
}

float get_asteroid_speed_for_difficulty(void) {
    return game.difficulty_multiplier;
}