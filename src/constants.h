#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <t3d/t3dmath.h>

// =============================================================================
// Display
// =============================================================================

#define SCREEN_WIDTH       320
#define SCREEN_HEIGHT      240

// =============================================================================
// Camera
// =============================================================================

#define CAM_DISTANCE       125.0f
#define CAM_ANGLE_PITCH    45.264f
#define CAM_ANGLE_YAW      135.0f
#define CAM_DEFAULT_FOV    100.0f
#define CAM_NEAR_PLANE     1.0f
#define CAM_FAR_PLANE      2000.0f
#define CAM_ASPECT_RATIO   ((float)SCREEN_WIDTH / (float)SCREEN_HEIGHT)
#define CAM_FOLLOW_SPEED   10.0f
#define CAM_ROTATION_SPEED 120.0f

// =============================================================================
// Playable Area
// =============================================================================

#define PLAY_AREA_HALF_X   832.0f
#define PLAY_AREA_HALF_Z   640.0f
#define PLAY_AREA_SIZE     840.0f

// =============================================================================
// Boundary Wall
// =============================================================================

#define WALL_FADE_START    50.0f   // Distance from edge where wall starts fading in
#define WALL_FADE_END      10.0f    // Distance from edge where wall is fully visible
#define WALL_HEIGHT        10.0f    // Y position of wall

// =============================================================================
// Drone
// =============================================================================

#define DEFAULT_HEIGHT         10.0f
#define DRONE_ROTATION_SPEED   5.0f
#define DRONE_MOVE_SPEED       1.0f
#define DRONE_ARRIVE_THRESHOLD 1.0f
#define DRONE_MAX_RESOURCES    30.0f

// =============================================================================
// Cursor / Ship
// =============================================================================

#define CURSOR_SPEED           5.0f
#define CURSOR_DEADZONE        15.0f
#define CURSOR_HEIGHT          10.0f
#define CURSOR_MAX_HEALTH      100.0f
#define CURSOR_RESOURCE_CAPACITY 100.0f
#define CURSOR_THRUST          10.0f
#define CURSOR_DRAG            1.110f
#define CURSOR_MAX_SPEED       300.0f
#define CURSOR_MAX_FUEL        100.0f
#define FPS_ROTATION_SPEED     3.0f
#define KNOCKBACK_STRENGTH     500.5f

// =============================================================================
// Asteroid
// =============================================================================

#define ASTEROID_BOUND_X   1000.0f
#define ASTEROID_BOUND_Z   800.0f
#define ASTEROID_PADDING   100.0f

// =============================================================================
// Resource
// =============================================================================

#define RESOURCE_BOUND_X   800.0f
#define RESOURCE_BOUND_Z   600.0f
#define RESOURCE_PADDING   100.0f
#define RESOURCE_VALUE     100.0f
#define RESOURCE_POOL_MAX  1000.0f
#define RESOURCE_POOL_MIN  000.0f
#define MINING_RATE        5.0f
#define DRONE_MINING_RATE  2.50f

// =============================================================================
// Station
// =============================================================================

#define STATION_DECAY_RATE     0.02f
#define STATION_MAX_HEALTH     100.0f
#define CURSOR_IFRAME_DURATION 1.0f

// =============================================================================
// Combat
// =============================================================================

#define DAMAGE_MULTIPLIER        0.001f
#define VALUE_MULTIPLIER         20.0f
#define MAX_DAMAGE               20.0f
#define MAX_COLOR_FLASHES        8
#define SHIP_DAMAGE_MULTIPLIER   3.0f
#define SPAWN_INVINCIBILITY_TIME 3.0f

// =============================================================================
// Debug UI
// =============================================================================

#define DEBUG_TEXT_X       16
#define DEBUG_TEXT_Y_START 50
#define DEBUG_LINE_HEIGHT  10

// =============================================================================
// Math
// =============================================================================

#define TWO_PI             (2.0f * T3D_PI)

// =============================================================================
// Colors
// =============================================================================

#define COLOR_MAP          RGBA32(255, 255, 255, 55)
#define COLOR_CURSOR       RGBA32(240, 240, 240, 255)
#define COLOR_DRONE        RGBA32(255, 0, 155, 255)
#define COLOR_STATION      RGBA32(220, 220, 220, 255)
#define COLOR_TILE         RGBA32(255, 149, 5, 250)
#define COLOR_ASTEROID     RGBA32(220, 0, 115, 255)
#define COLOR_AMBIENT      RGBA32(162, 204, 96, 155)
#define COLOR_SPARKS       RGBA32(255, 74, 28, 255)
#define COLOR_RESOURCE     RGBA32(81, 229, 255, 255)
#define COLOR_WARNING      RGBA32(255, 0, 0, 200)
#define COLOR_FLAME        RGBA32(255, 74, 28, 200)
#define COLOR_FUEL_BAR     RGBA32(138, 0, 196, 255)


// =============================================================================
// Font IDs
// =============================================================================

#define FONT_CUSTOM        2
#define FONT_ICON          3


// =============================================================================
// Menu Options
// =============================================================================

#define MENU_OPTION_RESUME   0
#define MENU_OPTION_HIRES    1
#define MENU_OPTION_AUDIO    2
#define MENU_OPTION_30HZ     3
#define MENU_OPTION_BG       4
#define MENU_OPTIONS_COUNT   5

// =============================================================================
// Fixed Timestep
// =============================================================================

#define FIXED_TIMESTEP (1.0f / 60.0f)

// =============================================================================
// Background
// =============================================================================

#define BG_WIDTH  1024
#define BG_HEIGHT 240

// =============================================================================
// Asteroid Speed (base values, scaled by difficulty)
// =============================================================================

#define ASTEROID_MIN_SCALE       0.3f
#define ASTEROID_MAX_SCALE       1.3f
#define ASTEROID_BASE_MIN_SPEED  100.0f
#define ASTEROID_BASE_MAX_SPEED  200.0f

// Distance-based culling (from cursor position)
#define ASTEROID_DRAW_DISTANCE_SQ    (1600.0f * 1600.0f)  // Don't draw beyond 1600 units
#define ASTEROID_ROTATE_DISTANCE_SQ  (1200.0f * 1200.0f)  // Don't rotate beyond 1200 units

// =============================================================================
// Deflect
// =============================================================================

#define DEFLECT_FUEL_COST  5.0f

#endif // CONSTANTS_H