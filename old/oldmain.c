#include <libdragon.h>
#include <t3d/t3d.h>
#include <t3d/t3dmath.h>
#include <t3d/t3dmodel.h>
#include <t3d/t3ddebug.h>
#include <rdpq.h>
#include <math.h>

// =============================================================================
// Configuration Constants
// =============================================================================

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

// Drone
#define DEFAULT_HEIGHT        10.0f
#define DRONE_ROTATION_SPEED  5.0f
#define DRONE_MOVE_SPEED      1.0f
#define DRONE_ARRIVE_THRESHOLD 1.0f

// Cursor
#define CURSOR_SPEED       5.0f
#define CURSOR_DEADZONE    10.0f
#define CURSOR_HEIGHT      10.0f

// Asteroid
#define ASTEROID_BOUND_X   800.0f
#define ASTEROID_BOUND_Z   700.0f
#define ASTEROID_PADDING   100.0f

// Debug UI
#define DEBUG_TEXT_X       16
#define DEBUG_TEXT_Y_START 50
#define DEBUG_LINE_HEIGHT  10

// Math
#define TWO_PI             (2.0f * T3D_PI)

// Colors
#define COLOR_MAP          RGBA32(100, 100, 100, 105)
#define COLOR_CURSOR       RGBA32(255, 0, 255, 200)
#define COLOR_DRONE        RGBA32(255, 255, 0, 255)
#define COLOR_STATION      RGBA32(100, 100, 100, 255)
#define COLOR_RING         RGBA32(150, 150, 150, 255)
#define COLOR_TILE         RGBA32(0, 100, 200, 180)
#define COLOR_ASTEROID     RGBA32(101, 67, 33, 255)

// =============================================================================
// Type Definitions
// =============================================================================

typedef enum {
    DRAW_SHADED,
    DRAW_TEXTURED_LIT
} DrawType;

typedef struct {
    T3DVec3 position;
    T3DVec3 target;
    float rotation_y;
} Camera;

typedef struct {
    T3DModel *model;
    T3DMat4FP *matrix;
    T3DVec3 position;
    T3DVec3 velocity;
    color_t color;
    float scale;
    float rotation_x;
    float rotation_y;
    float rotation_z;
    float speed;
    DrawType draw_type;
} Entity;

typedef enum {
    ENTITY_STATION,
    ENTITY_CURSOR,
    ENTITY_DRONE,
    ENTITY_TILE,
    ENTITY_RING,
    ENTITY_GRID,
    ENTITY_COUNT
} EntityID;

typedef enum {
    ASTEROID_A1,
    ASTEROID_A2,
    ASTEROID_A3,
    ASTEROID_A4,
    ASTEROID_A5,
    ASTEROID_A6,
    ASTEROID_A7,
    ASTEROID_A8,
    ASTEROID_COUNT
} AsteroidID;

// =============================================================================
// Global State
// =============================================================================

// Camera
static Camera camera = {
    .position = {{0.0f, 0.0f, 0.0f}},
    .target = {{0.0f, 0.0f, 0.0f}},
    .rotation_y = 0.0f
};

// Entities
static Entity entities[ENTITY_COUNT];
static Entity asteroids[ASTEROID_COUNT];
static Entity *cursor_entity = NULL;

// Cursor
static T3DVec3 cursor_position = {{0.0f, CURSOR_HEIGHT, 0.0f}};
static bool cursor_intersecting = false;

// Drone Movement
static bool move_drone = false;
static float target_rotation = 0.0f;
static T3DVec3 target_position;

// Debug/FPS Stats
static bool render_debug = false;
static float fps_min = 9999.0f;
static float fps_max = 0.0f;
static float fps_current = 0.0f;
static float fps_avg = 0.0f;
static float fps_total = 0.0f;
static int fps_frame_count = 0;

// =============================================================================
// Helper Functions
// =============================================================================

static inline float get_time_s(void) {
    return (float)((double)get_ticks_us() / 1000000.0);
}

static inline float clampf(float val, float min_val, float max_val) {
    return fmaxf(fminf(val, max_val), min_val);
}

static inline float normalize_angle(float angle) {
    while (angle > T3D_PI) angle -= TWO_PI;
    while (angle < -T3D_PI) angle += TWO_PI;
    return angle;
}

static inline float randomize_float(float min, float max) {
    return min + ((float)rand() / (float)RAND_MAX) * (max - min);
}

static const char* get_compass_direction(float angle) {
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

// =============================================================================
// Initialization
// =============================================================================

// resolution_t custom_res = {
//   .width = 640,
//   .height = 480,
//   .interlaced = false
// };
static void init_subsystems(void) {
    debug_init_isviewer();
    debug_init_usblog();
    asset_init_compression(2);
    dfs_init(DFS_DEFAULT_LOCATION);
    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 3, GAMMA_NONE, FILTERS_RESAMPLE);
    // display_init(custom_res, DEPTH_16_BPP, 3, GAMMA_NONE, FILTERS_RESAMPLE);
    // display_set_fps_limit(30);
    // display_set_fps_limit(60);
    rdpq_init();
    joypad_init();
    t3d_init((T3DInitParams){});
    rdpq_text_register_font(
        FONT_BUILTIN_DEBUG_MONO,
        rdpq_font_load_builtin(FONT_BUILTIN_DEBUG_MONO)
    );
}

// =============================================================================
// Entity Management
// =============================================================================

static Entity create_entity(const char *model_path, T3DVec3 position, float scale, color_t color, DrawType draw_type) {
    Entity entity = {
        .model = t3d_model_load(model_path),
        .matrix = malloc_uncached(sizeof(T3DMat4FP)),
        .position = position,
        .velocity = {{0.0f, 0.0f, 0.0f}},
        .scale = scale,
        .color = color,
        .rotation_x = 0.0f,
        .rotation_y = 0.0f,
        .rotation_z = 0.0f,
        .speed = 1.0f,
        .draw_type = draw_type
    };
    return entity;
}

static void update_entity_matrix(Entity *entity) {
    float scale[3] = {entity->scale, entity->scale, entity->scale};
    float rotation[3] = {entity->rotation_x, entity->rotation_y, entity->rotation_z};
    t3d_mat4fp_from_srt_euler(entity->matrix, scale, rotation, entity->position.v);
}

static void update_entity_matrices(Entity *entity_array, int count) {
    for (int i = 0; i < count; i++) {
        update_entity_matrix(&entity_array[i]);
    }
}

static void rotate_entity(Entity *entity, float delta_time, float speed) {
    entity->rotation_y += delta_time * speed;
    if (entity->rotation_y > TWO_PI) {
        entity->rotation_y -= TWO_PI;
    }
}

static void free_entity(Entity *entity) {
    if (entity->model) {
        t3d_model_free(entity->model);
        entity->model = NULL;
    }
    if (entity->matrix) {
        free_uncached(entity->matrix);
        entity->matrix = NULL;
    }
}

static void free_all_entities(Entity *entity_array, int count) {
    for (int i = 0; i < count; i++) {
        free_entity(&entity_array[i]);
    }
}

// =============================================================================
// Drawing
// =============================================================================

static void draw_entity(Entity *entity) {
    t3d_matrix_push(entity->matrix);
    rdpq_set_prim_color(entity->color);

    if (entity->draw_type == DRAW_SHADED) {
        rdpq_mode_combiner(RDPQ_COMBINER1((PRIM, 0, SHADE, 0), (PRIM, 0, SHADE, 0)));
    } else {
        rdpq_mode_combiner(RDPQ_COMBINER1((TEX0, 0, SHADE, 0), (TEX0, 0, SHADE, 0)));
    }

    t3d_model_draw(entity->model);
    t3d_matrix_pop(1);
}

static void draw_entities(Entity *entity_array, int count) {
    for (int i = 0; i < count; i++) {
        draw_entity(&entity_array[i]);
    }
}

// =============================================================================
// Camera
// =============================================================================

static void reset_cam_yaw(float *cam_yaw) {
    *cam_yaw = CAM_ANGLE_YAW;
}

static void teleport_to_position(float x, float z, float *cam_yaw) {
    camera.target.v[0] = x;
    camera.target.v[2] = z;
    cursor_position.v[0] = x;
    cursor_position.v[2] = z;
    reset_cam_yaw(cam_yaw);
}

static void update_camera(T3DViewport *viewport, float cam_yaw, float delta_time) {
    float pitch_rad = T3D_DEG_TO_RAD(CAM_ANGLE_PITCH);
    float yaw_rad = T3D_DEG_TO_RAD(cam_yaw);

    float horizontal_dist = CAM_DISTANCE * cosf(pitch_rad);
    float vertical_dist = CAM_DISTANCE * sinf(pitch_rad);

    float follow_speed = CAM_FOLLOW_SPEED * delta_time;
    if (follow_speed > 1.0f) follow_speed = 1.0f;

    camera.target.v[0] += (cursor_position.v[0] - camera.target.v[0]) * follow_speed;
    camera.target.v[2] += (cursor_position.v[2] - camera.target.v[2]) * follow_speed;

    camera.target.v[0] = clampf(camera.target.v[0], -PLAY_AREA_HALF_X, PLAY_AREA_HALF_X);
    camera.target.v[2] = clampf(camera.target.v[2], -PLAY_AREA_HALF_Z, PLAY_AREA_HALF_Z);

    camera.position.v[0] = camera.target.v[0] + horizontal_dist * sinf(yaw_rad);
    camera.position.v[1] = camera.target.v[1] + vertical_dist;
    camera.position.v[2] = camera.target.v[2] + horizontal_dist * cosf(yaw_rad);

    t3d_viewport_set_perspective(viewport, T3D_DEG_TO_RAD(CAM_DEFAULT_FOV),
                                  CAM_ASPECT_RATIO, CAM_NEAR_PLANE, CAM_FAR_PLANE);

    T3DVec3 up = {{0, 1, 0}};
    t3d_viewport_look_at(viewport, &camera.position, &camera.target, &up);
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

    if (!held.r) {
        float cursor_speed = CURSOR_SPEED * delta_time;
        float yaw_rad = T3D_DEG_TO_RAD(cam_yaw);
        float cos_yaw = cosf(yaw_rad);
        float sin_yaw = sinf(yaw_rad);

        float rotated_x = (joypad.stick_x * cos_yaw - joypad.stick_y * sin_yaw) * cursor_speed;
        float rotated_z = (joypad.stick_x * sin_yaw + joypad.stick_y * cos_yaw) * cursor_speed;

        cursor_position.v[0] += rotated_x;
        cursor_position.v[2] -= rotated_z;

        float stick_magnitude = sqrtf(joypad.stick_x * joypad.stick_x +
                                      joypad.stick_y * joypad.stick_y);

        if (stick_magnitude > CURSOR_DEADZONE && cursor_entity) {
            cursor_entity->rotation_y = atan2f(-rotated_x, -rotated_z);
        }
    }

    if (cursor_entity) {
        cursor_entity->position = cursor_position;
        clamp_cursor_position();
    }
}

// =============================================================================
// Drone Movement
// =============================================================================

static void move_drone_towards_cursor(Entity *drone, float delta_time) {
    if (cursor_intersecting) return;

    float dx = target_position.v[0] - drone->position.v[0];
    float dz = target_position.v[2] - drone->position.v[2];
    float distance_sq = dx * dx + dz * dz;

    if (distance_sq < DRONE_ARRIVE_THRESHOLD * DRONE_ARRIVE_THRESHOLD) {
        move_drone = false;
        return;
    }

    float angle_diff = normalize_angle(target_rotation - drone->rotation_y);
    float rotation_factor = DRONE_ROTATION_SPEED * delta_time;
    if (rotation_factor > 1.0f) rotation_factor = 1.0f;

    if (fabsf(angle_diff) < 0.01f) {
        drone->rotation_y = target_rotation;
    } else {
        drone->rotation_y += angle_diff * rotation_factor;
        drone->rotation_y = normalize_angle(drone->rotation_y);
    }

    float move_factor = DRONE_MOVE_SPEED * delta_time;
    drone->position.v[0] += dx * move_factor;
    drone->position.v[2] += dz * move_factor;
}

static void update_tile_visibility(Entity *tile) {
    tile->position.v[0] = move_drone ? target_position.v[0] : 0.0f;
    tile->position.v[1] = move_drone ? 8.0f : 1000.0f;
    tile->position.v[2] = move_drone ? target_position.v[2] : 0.0f;
}

// =============================================================================
// Asteroids
// =============================================================================

static void reset_asteroid(Entity *asteroid) {
    int edge = rand() % 4;

    asteroid->speed = randomize_float(200.0f, 450.0f);
    asteroid->scale = randomize_float(0.1f, 1.3f);

    switch (edge) {
        case 0:  // North
            asteroid->position.v[0] = randomize_float(-ASTEROID_BOUND_X, ASTEROID_BOUND_X);
            asteroid->position.v[2] = -ASTEROID_BOUND_Z;
            asteroid->velocity.v[0] = randomize_float(-0.5f, 0.5f);
            asteroid->velocity.v[2] = randomize_float(0.3f, 1.0f);
            break;
        case 1:  // South
            asteroid->position.v[0] = randomize_float(-ASTEROID_BOUND_X, ASTEROID_BOUND_X);
            asteroid->position.v[2] = ASTEROID_BOUND_Z;
            asteroid->velocity.v[0] = randomize_float(-0.5f, 0.5f);
            asteroid->velocity.v[2] = randomize_float(-1.0f, -0.3f);
            break;
        case 2:  // East
            asteroid->position.v[0] = ASTEROID_BOUND_X;
            asteroid->position.v[2] = randomize_float(-ASTEROID_BOUND_Z, ASTEROID_BOUND_Z);
            asteroid->velocity.v[0] = randomize_float(-1.0f, -0.3f);
            asteroid->velocity.v[2] = randomize_float(-0.5f, 0.5f);
            break;
        case 3:  // West
            asteroid->position.v[0] = -ASTEROID_BOUND_X;
            asteroid->position.v[2] = randomize_float(-ASTEROID_BOUND_Z, ASTEROID_BOUND_Z);
            asteroid->velocity.v[0] = randomize_float(0.3f, 1.0f);
            asteroid->velocity.v[2] = randomize_float(-0.5f, 0.5f);
            break;
    }

    float len = sqrtf(asteroid->velocity.v[0] * asteroid->velocity.v[0] +
                      asteroid->velocity.v[2] * asteroid->velocity.v[2]);
    if (len > 0) {
        asteroid->velocity.v[0] /= len;
        asteroid->velocity.v[2] /= len;
    }
}

static void move_asteroid(Entity *asteroid, float delta_time) {
    asteroid->velocity.v[0] += randomize_float(-0.1f, 0.1f) * delta_time;
    asteroid->velocity.v[2] += randomize_float(-0.1f, 0.1f) * delta_time;

    float len = sqrtf(asteroid->velocity.v[0] * asteroid->velocity.v[0] +
                      asteroid->velocity.v[2] * asteroid->velocity.v[2]);
    if (len > 0.1f) {
        asteroid->velocity.v[0] /= len;
        asteroid->velocity.v[2] /= len;
    }

    float move_amount = asteroid->speed * delta_time;
    asteroid->position.v[0] += asteroid->velocity.v[0] * move_amount;
    asteroid->position.v[2] += asteroid->velocity.v[2] * move_amount;

    asteroid->rotation_x += delta_time * asteroid->speed * 0.05f;
    asteroid->rotation_y += delta_time * asteroid->speed * 0.03f;
    asteroid->rotation_z += delta_time * asteroid->speed * 0.02f;

    bool out_of_bounds =
        asteroid->position.v[0] < -(ASTEROID_BOUND_X + ASTEROID_PADDING) ||
        asteroid->position.v[0] > (ASTEROID_BOUND_X + ASTEROID_PADDING) ||
        asteroid->position.v[2] < -(ASTEROID_BOUND_Z + ASTEROID_PADDING) ||
        asteroid->position.v[2] > (ASTEROID_BOUND_Z + ASTEROID_PADDING);

    if (out_of_bounds) {
        reset_asteroid(asteroid);
    }
}

static void update_asteroids(float delta_time) {
    for (int i = 0; i < ASTEROID_COUNT; i++) {
        move_asteroid(&asteroids[i], delta_time);
    }
}

// =============================================================================
// Input
// =============================================================================

static void reset_fps_stats(void) {
    fps_min = 9999.0f;
    fps_max = 0.0f;
    fps_total = 0.0f;
    fps_frame_count = 0;
    fps_avg = 0.0f;
}

static void process_input(float delta_time, float *cam_yaw) {
    joypad_inputs_t joypad = joypad_get_inputs(JOYPAD_PORT_1);
    joypad_buttons_t held = joypad_get_buttons_held(JOYPAD_PORT_1);
    joypad_buttons_t pressed = joypad_get_buttons_pressed(JOYPAD_PORT_1);

    // Camera rotation
    if (held.r) {
        float rotation_speed = CAM_ROTATION_SPEED * delta_time;
        if (joypad.stick_x < 0) {
            *cam_yaw -= rotation_speed;
        } else if (joypad.stick_x > 0) {
            *cam_yaw += rotation_speed;
        }
    }

    // Reset FPS stats
    if (pressed.l) {
        reset_fps_stats();
    }

    // Teleport to preset positions
    if (held.r) {
        if (pressed.c_up) teleport_to_position(-576.6f, 276.0f, cam_yaw);
        else if (pressed.c_right) teleport_to_position(-489.6f, -470.0f, cam_yaw);
        else if (pressed.c_down) teleport_to_position(690.6f, -519.0f, cam_yaw);
        else if (pressed.c_left) teleport_to_position(730.6f, 378.0f, cam_yaw);
        else if (pressed.z) teleport_to_position(0.0f, 0.0f, cam_yaw);
    }

    // Toggle debug UI
    if (pressed.start) {
        render_debug = !render_debug;
    }

    // Command drone to move
    if (pressed.a && cursor_entity) {
        target_rotation = cursor_entity->rotation_y;
        target_position = cursor_position;
        move_drone = true;
    }
}

// =============================================================================
// Rendering
// =============================================================================

static void setup_lighting(void) {
    uint8_t color_ambient[4] = {0x90, 0x90, 0x90, 0xFF};
    uint8_t color_directional[4] = {0xFF, 0xFF, 0xFF, 0xFF};
    T3DVec3 light_dir = {{1.0f, 1.0f, 1.0f}};
    t3d_vec3_norm(&light_dir);

    t3d_light_set_ambient(color_ambient);
    t3d_light_set_directional(0, color_directional, &light_dir);
    t3d_light_set_count(1);
}

static void update_fps_stats(float delta_time) {
    fps_current = 1.0f / delta_time;

    if (fps_current > 1.0f && fps_current < 1000.0f) {
        if (fps_current < fps_min) fps_min = fps_current;
        if (fps_current > fps_max) fps_max = fps_current;

        fps_total += fps_current;
        fps_frame_count++;
        fps_avg = fps_total / fps_frame_count;
    }
}

static void render_debug_ui(void) {
    rdpq_sync_pipe();

    heap_stats_t stats;
    sys_get_heap_stats(&stats);

    int y = DEBUG_TEXT_Y_START;

    rdpq_text_printf(NULL, FONT_BUILTIN_DEBUG_MONO, DEBUG_TEXT_X, y,
                     "fps: %.0f avg: %.0f", fps_current, fps_avg);

    y += DEBUG_LINE_HEIGHT;
    rdpq_text_printf(NULL, FONT_BUILTIN_DEBUG_MONO, DEBUG_TEXT_X, y,
                     "min: %.0f max: %.0f", fps_min, fps_max);

    y += DEBUG_LINE_HEIGHT;
    rdpq_text_printf(NULL, FONT_BUILTIN_DEBUG_MONO, DEBUG_TEXT_X, y,
                     "ram: %dKB / %dKB", stats.used / 1024, stats.total / 1024);

    y += DEBUG_LINE_HEIGHT;
    rdpq_text_printf(NULL, FONT_BUILTIN_DEBUG_MONO, DEBUG_TEXT_X, y,
                     "cam: %.0f %.0f %.0f",
                     camera.position.v[0], camera.position.v[1], camera.position.v[2]);

    y += DEBUG_LINE_HEIGHT;
    rdpq_text_printf(NULL, FONT_BUILTIN_DEBUG_MONO, DEBUG_TEXT_X, y,
                     "cursor: %.0f %.0f", cursor_position.v[0], cursor_position.v[2]);

    if (cursor_entity) {
        y += DEBUG_LINE_HEIGHT;
        rdpq_text_printf(NULL, FONT_BUILTIN_DEBUG_MONO, DEBUG_TEXT_X, y,
                         "dir: %s", get_compass_direction(cursor_entity->rotation_y));
    }

    y += DEBUG_LINE_HEIGHT;
    rdpq_text_printf(NULL, FONT_BUILTIN_DEBUG_MONO, DEBUG_TEXT_X, y,
                     "drone: %s", move_drone ? "moving" : "idle");
}

static void render_frame(T3DViewport *viewport, sprite_t *background) {
    rdpq_attach(display_get(), display_get_zbuf());

    rdpq_set_mode_copy(false);
    rdpq_sprite_blit(background, 0, 0, NULL);

    t3d_frame_start();
    t3d_viewport_attach(viewport);
    t3d_screen_clear_depth();
    setup_lighting();

    draw_entities(entities, ENTITY_COUNT);
    draw_entities(asteroids, ASTEROID_COUNT);

    if (render_debug) {
        render_debug_ui();
    }

    rdpq_detach_show();
}

// =============================================================================
// Main
// =============================================================================

int main(void) {
    init_subsystems();
    T3DViewport viewport = t3d_viewport_create();

    sprite_t *background = sprite_load("rom:/spacesmdark.sprite");

    // Create entities
    entities[ENTITY_CURSOR] = create_entity("rom:/cursor.t3dm", cursor_position, 0.0625f, COLOR_CURSOR, DRAW_TEXTURED_LIT);
    entities[ENTITY_DRONE] = create_entity("rom:/ship.t3dm", (T3DVec3){{20.0f, DEFAULT_HEIGHT, 29.0f}}, 0.5f, COLOR_DRONE, DRAW_TEXTURED_LIT);
    entities[ENTITY_TILE] = create_entity("rom:/tile2.t3dm", (T3DVec3){{0, 1000, 0}}, 1.0f, COLOR_TILE, DRAW_TEXTURED_LIT);
    entities[ENTITY_STATION] = create_entity("rom:/station2.t3dm", (T3DVec3){{0, DEFAULT_HEIGHT, 0}}, 1.00f, COLOR_STATION, DRAW_TEXTURED_LIT);
    entities[ENTITY_RING] = create_entity("rom:/stationring.t3dm", (T3DVec3){{0, DEFAULT_HEIGHT, 0}}, 1.00f, COLOR_RING, DRAW_SHADED);
    entities[ENTITY_GRID] = create_entity("rom:/gridalpha2.t3dm", (T3DVec3){{0, 1, 0}}, 1.0f, COLOR_MAP, DRAW_SHADED);

    // Create asteroids
    for (int i = 0; i < ASTEROID_COUNT; i++) {
        asteroids[i] = create_entity("rom:/asteroid.t3dm", (T3DVec3){{0, 10, 0}},
                                      randomize_float(0.1f, 1.3f), COLOR_ASTEROID, DRAW_SHADED);
        reset_asteroid(&asteroids[i]);
        asteroids[i].position.v[0] = randomize_float(-ASTEROID_BOUND_X, ASTEROID_BOUND_X);
        asteroids[i].position.v[2] = randomize_float(-ASTEROID_BOUND_Z, ASTEROID_BOUND_Z);
    }

    cursor_entity = &entities[ENTITY_CURSOR];

    float last_time = get_time_s() - (1.0f / 60.0f);
    float cam_yaw = CAM_ANGLE_YAW;

    for (;;) {
        float current_time = get_time_s();
        float delta_time = current_time - last_time;
        last_time = current_time;

        // Input
        joypad_poll();
        process_input(delta_time, &cam_yaw);

        // Update
        update_cursor(delta_time, cam_yaw);
        update_camera(&viewport, cam_yaw, delta_time);
        update_tile_visibility(&entities[ENTITY_TILE]);

        if (move_drone) {
            move_drone_towards_cursor(&entities[ENTITY_DRONE], delta_time);
        }

        update_asteroids(delta_time);
        rotate_entity(&entities[ENTITY_RING], delta_time, 0.250f);

        update_entity_matrices(entities, ENTITY_COUNT);
        update_entity_matrices(asteroids, ASTEROID_COUNT);

        // Render
        update_fps_stats(delta_time);
        render_frame(&viewport, background);
    }

    free_all_entities(entities, ENTITY_COUNT);
    free_all_entities(asteroids, ASTEROID_COUNT);
    t3d_destroy();

    return 0;
}