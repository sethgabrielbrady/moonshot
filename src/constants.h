#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <t3d/t3dmath.h>

// Display
#define SCREEN_WIDTH       320
#define SCREEN_HEIGHT      240

// Camera
#define CAM_DISTANCE       125.0f
#define CAM_ANGLE_PITCH    45.264f
#define CAM_ANGLE_YAW      135.0f
#define CAM_DEFAULT_FOV    100.0f
#define CAM_NEAR_PLANE     1.0f
#define CAM_FAR_PLANE      2000.0f
#define CAM_ASPECT_RATIO   ((float)SCREEN_WIDTH / (float)SCREEN_HEIGHT)
#define CAM_FOLLOW_SPEED   10.0f
#define CAM_ROTATION_SPEED 120.0f

// Playable Area
#define PLAY_AREA_HALF_X   832.0f
#define PLAY_AREA_HALF_Z   640.0f
#define PLAY_AREA_SIZE     840.0f  // Used for particle boundaries


// Drone
#define DEFAULT_HEIGHT        10.0f
#define DRONE_ROTATION_SPEED  5.0f
#define DRONE_MOVE_SPEED      1.0f
#define DRONE_ARRIVE_THRESHOLD 1.0f
#define DRONE_MAX_RESOURCES      50.0f // not actually health b

// Cursor
#define CURSOR_SPEED       5.0f
#define CURSOR_DEADZONE    15.0f
#define CURSOR_HEIGHT      10.0f
#define CURSOR_MAX_HEALTH  100.0f
#define CURSOR_RESOURCE_CAPACITY 100.0f

// Asteroid
#define ASTEROID_BOUND_X   1000.0f
#define ASTEROID_BOUND_Z   800.0f
#define ASTEROID_PADDING   100.0f


//Resource
#define RESOURCE_BOUND_X   600.0f
#define RESOURCE_BOUND_Z   500.0f
#define RESOURCE_PADDING   100.0f
#define RESOURCE_VALUE     100.0f // This will get multiplied by the scale to determine value
#define RESOURCE_POOL_MAX  1000.0f
#define RESOURCE_POOL_MIN  000.0f
#define MINING_RATE        10.0f // per second
#define DRONE_MINING_RATE  2.50f  // per second

// Debug UI
#define DEBUG_TEXT_X       16
#define DEBUG_TEXT_Y_START 50
#define DEBUG_LINE_HEIGHT  10

// Math
#define TWO_PI             (2.0f * T3D_PI)

// Colors
#define COLOR_MAP          RGBA32(255, 255, 255, 55)
#define COLOR_CURSOR       RGBA32(240, 240, 240, 255)
#define COLOR_DRONE        RGBA32(255, 0, 155, 255)
#define COLOR_STATION      RGBA32(220, 220, 220, 255)
#define COLOR_RING         RGBA32(150, 150, 150, 255)
#define COLOR_TILE         RGBA32(255, 149, 5, 120)
#define COLOR_ASTEROID     RGBA32(108, 136, 64, 255)
#define COLOR_SPARKS       RGBA32(255, 220, 0, 255)
// #define COLOR_ASTEROID     RGBA32(78, 56, 34, 255)

#define COLOR_RESOURCE     RGBA32(27, 154, 170, 255)


 #define STATION_DECAY_RATE 0.02f // per second
 #define MAX_COLOR_FLASHES 8
 #define DAMAGE_MULTIPLIER 0.001f
 #define VALUE_MULTIPLIER 20.0f
 #define STATION_MAX_HEALTH 100.0f
 #define MAX_DAMAGE 20.0f


 // Cursor physics
#define CURSOR_THRUST        10.0f      // Acceleration when B is held
#define CURSOR_DRAG          1.110f      // How quickly velocity decays
#define CURSOR_MAX_SPEED     300.0f    // Maximum velocity
#define FPS_ROTATION_SPEED   3.0f  // Adjust for faster/slower rotation

#define KNOCKBACK_STRENGTH   500.5f


// background
#define BG_WIDTH  1024
#define BG_HEIGHT 240

#define FONT_CUSTOM           2
// #define FONT_CUSTOM_TWO       3

static rdpq_font_t *custom_font = NULL;
// static rdpq_font_t *custom_font_two = NULL;




static bool game_paused = false;
static int menu_selection = 0;
#define MENU_OPTION_RESUME   0
#define MENU_OPTION_CAMERA   1
#define MENU_OPTION_DEBUG    2
#define MENU_OPTION_AUDIO    3
#define MENU_OPTION_30HZ     4
#define MENU_OPTION_BG       5
#define MENU_OPTIONS_COUNT   6


static bool limit30hz = false;
static int blink_timer = 0;
static int station_last_damage = 0;
static int cursor_last_damage = 0;

static sprite_t *station_icon = NULL;
static sprite_t *tile_icon = NULL;
static sprite_t *drill_icon = NULL;
static sprite_t *drone_icon = NULL;
static sprite_t *ship_icon = NULL;
static sprite_t *health_icon = NULL;



// Fixed timestep variables
#define FIXED_TIMESTEP (1.0f / 60.0f)
static float accumulator = 0.0f;

#endif