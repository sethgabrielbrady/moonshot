#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <libdragon.h>
#include <t3d/t3d.h>
#include <t3d/t3dmath.h>

// =============================================================================
// Game State Enum
// =============================================================================

typedef enum {
    STATE_TITLE,
    STATE_COUNTDOWN,
    STATE_PLAYING,
    STATE_PAUSED,
    STATE_GAME_OVER,
    STATE_STATION_EXPLODING
} GameState;

// =============================================================================
// Centralized Game State Structure
// =============================================================================

typedef struct {
    // Core game state
    GameState state;
    bool game_over;
    bool game_over_pause;

    // Menu
    int menu_selection;

    // Display/rendering options
    bool fps_mode;
    bool render_debug;
    bool hi_res_mode;
    bool show_fps;
    bool render_background_enabled;
    bool show_controls;        // Controls menu visible
    bool show_tutorial;        // Tutorial menu visible
    bool show_credits;         // Credits menu visible
    int fps_limit;          // 0=30fps, 1=60fps, 2=uncapped
    bool is_pal_system;

    // Audio
    int bgm_track;          // 0=off, 1=track1, 2=track2, 3=track3

    // Blink timer for UI animations
    int blink_timer;

    // Damage tracking for UI
    int station_last_damage;
    int cursor_last_damage;

    // Resource values
    int resource_val;
    int cursor_resource_val;
    int drone_resource_val;
    int highlighted_resource;

    // Mining state
    float cursor_mining_accumulated;
    float drone_mining_accumulated;
    int cursor_mining_resource;
    int drone_mining_resource;
    bool cursor_is_mining;
    bool drone_is_mining;

    // Drone state
    bool move_drone;
    bool drone_heal;
    bool drone_collecting_resource;
    bool drone_moving_to_resource;
    bool drone_moving_to_station;
    bool drone_full;
    T3DVec3 drone_target_position;
    float drone_target_rotation;

    // Tile state
    int tile_following_resource;
    float tile_scale_multiplier;

    // Cursor state
    float cursor_scale_multiplier;
    T3DVec3 cursor_position;
    T3DVec3 cursor_velocity;

    // Camera
    float cam_yaw;

    // Iframe timers
    float cursor_iframe_timer;

    // Frame counting
    int frame_count;

    // Time accumulators for throttled updates
    float particle_render_timer;
    // float ambient_particle_timer;
    float asteroid_matrix_timer;
    float collision_timer;

    // Fixed timestep accumulator
    float accumulator;

    bool reset;
        // Deflection state
    float deflect_timer;        // Time remaining in deflection window (500ms)
    bool deflect_active;        // Is deflection window active
    int deflect_count;          // Successful deflection counter
    bool disabled_controls;
    int player_lives;

    float death_timer;          // Counts up to 10 seconds
    bool death_timer_active;    // Is timer running

    float go_display_timer;     // Timer for "GO" text after successful heal

    int accumulated_credits;    // Credits accumulated during play session
    float ship_fuel;            // Current ship fuel level
    bool ship_acceleration;     // Is ship currently accelerating

    // Difficulty progression
    float game_time;            // Total elapsed game time in seconds
    float difficulty_multiplier; // Scales from 1.0 upward based on time

    // Countdown
    float countdown_timer;      // Counts down from 3.0 to 0
    bool hauled_resources;
    float hauled_resources_timer;

    char status_message[64];
    float status_message_timer;

} GameStateData;

// =============================================================================
// Global Game State Instance
// =============================================================================

extern GameStateData game;

// =============================================================================
// Fonts (must be extern, defined in game_state.c)
// =============================================================================

extern rdpq_font_t *custom_font;
extern rdpq_font_t *icon_font;

// =============================================================================
// Sprites/Icons (must be extern, defined in game_state.c)
// =============================================================================

extern sprite_t *station_icon;
extern sprite_t *tile_icon;
extern sprite_t *drill_icon;
extern sprite_t *drone_icon;
extern sprite_t *drone_full_icon;
extern sprite_t *health_icon;

// =============================================================================
// Functions
// =============================================================================

void init_game_state(void);
void pause_game(void);
void unpause_game(void);
void set_game_over(void);
void update_difficulty(float delta_time);
float get_asteroid_speed_for_difficulty(void);




#endif // GAME_STATE_H