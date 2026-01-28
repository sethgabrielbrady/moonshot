#include "spawner.h"
#include "constants.h"
#include "utils.h"
#include "entity.h"
#include "game_state.h"
#include <rdpq.h>
#include <math.h>
#include <stdint.h>

// =============================================================================
// Shared Models (loaded once, reused by all instances)
// =============================================================================

static T3DModel *shared_asteroid_model = NULL;

// =============================================================================
// Pre-computed Rotation Lookup Tables
// =============================================================================

#define ROTATION_TABLE_SIZE 360
static float rotation_sin[ROTATION_TABLE_SIZE];
static float rotation_cos[ROTATION_TABLE_SIZE];
static bool rotation_tables_initialized = false;

static void init_rotation_tables(void) {
    if (rotation_tables_initialized) return;

    for (int i = 0; i < ROTATION_TABLE_SIZE; i++) {
        float angle = i * M_PI / 180.0f;
        rotation_sin[i] = sinf(angle);
        rotation_cos[i] = cosf(angle);
    }
    rotation_tables_initialized = true;
}

// =============================================================================
// Speed/Scale Configuration
// =============================================================================


static void get_asteroid_velocity_and_scale(Entity *asteroid, T3DVec3 *out_velocity) {
    // Get difficulty multiplier from game state (1.0 to 5.0)
    float difficulty = get_asteroid_speed_for_difficulty();

    // Scale speed by difficulty
    float min_speed = ASTEROID_BASE_MIN_SPEED * difficulty;
    float max_speed = ASTEROID_BASE_MAX_SPEED * difficulty;

    asteroid->speed = randomize_float(min_speed, max_speed);
    asteroid->scale = randomize_float(ASTEROID_MIN_SCALE, ASTEROID_MAX_SCALE);
}

static void get_resource_velocity_and_scale(Entity *resource, T3DVec3 *out_velocity) {
    resource->speed = randomize_float(5.0f, 20.0f);
    resource->scale = randomize_float(0.6f, 1.4f);
}

void scale_resource_based_on_value(Entity *resource) {
    resource->value = resource->scale * VALUE_MULTIPLIER;
}

// =============================================================================
// Entity Reset
// =============================================================================

void reset_entity(Entity *entity, EntityType type) {
    if (type == ASTEROID) {
        get_asteroid_velocity_and_scale(entity, &entity->velocity);
    }
    if (type == RESOURCE) {
        get_resource_velocity_and_scale(entity, &entity->velocity);
        entity->color = COLOR_RESOURCE;
        scale_resource_based_on_value(entity);
    }

    float bound_radius = (type == RESOURCE) ? RESOURCE_BOUND_X : ASTEROID_BOUND_X;

    // Spawn on circle edge at random angle
    float angle = randomize_float(0.0f, TWO_PI);
    entity->position.v[0] = cosf(angle) * bound_radius;
    entity->position.v[2] = sinf(angle) * bound_radius;

    // Velocity points toward center with some randomness
    float target_angle = angle + T3D_PI + randomize_float(-0.5f, 0.5f);
    entity->velocity.v[0] = cosf(target_angle);
    entity->velocity.v[2] = sinf(target_angle);

    // Normalize velocity using fast inverse sqrt
    float len_sq = entity->velocity.v[0] * entity->velocity.v[0] +
                   entity->velocity.v[2] * entity->velocity.v[2];
    if (len_sq > 0.0001f) {
        float inv_len = fast_inv_sqrt(len_sq);
        entity->velocity.v[0] *= inv_len;
        entity->velocity.v[2] *= inv_len;
    }
}

// =============================================================================
// Entity Movement
// =============================================================================

void move_entity(Entity *entity, float delta_time, EntityType type) {
    // Normalize velocity only if needed
    float len_sq = entity->velocity.v[0] * entity->velocity.v[0] +
                   entity->velocity.v[2] * entity->velocity.v[2];
    if (len_sq > 1.1f) {
        float inv_len = fast_inv_sqrt(len_sq);
        entity->velocity.v[0] *= inv_len;
        entity->velocity.v[2] *= inv_len;
    }

    float move_amount = entity->speed * delta_time;
    entity->position.v[0] += entity->velocity.v[0] * move_amount;
    entity->position.v[2] += entity->velocity.v[2] * move_amount;

    // Y rotation - only for nearby entities (optimization)
    if (type == ASTEROID) {
        float dx = entity->position.v[0] - game.cursor_position.v[0];
        float dz = entity->position.v[2] - game.cursor_position.v[2];
        float dist_sq = dx * dx + dz * dz;

        if (dist_sq < ASTEROID_ROTATE_DISTANCE_SQ) {
            entity->rotation.v[1] += move_amount * 0.03f;
            while (entity->rotation.v[1] >= 360.0f) entity->rotation.v[1] -= 360.0f;
            while (entity->rotation.v[1] < 0.0f) entity->rotation.v[1] += 360.0f;
        }
    } else {
        entity->rotation.v[1] += move_amount * 0.03f;
        while (entity->rotation.v[1] >= 360.0f) entity->rotation.v[1] -= 360.0f;
        while (entity->rotation.v[1] < 0.0f) entity->rotation.v[1] += 360.0f;
    }

    // Boundary check (circular)
    float bound_radius = ASTEROID_BOUND_X + ASTEROID_PADDING;
    float dist_sq = entity->position.v[0] * entity->position.v[0] +
                    entity->position.v[2] * entity->position.v[2];

    if (dist_sq > bound_radius * bound_radius) {
        reset_entity(entity, type);
    }
}

// =============================================================================
// Asteroid Functions
// =============================================================================

void update_asteroids(Entity *asteroids, int count, float delta_time) {
    for (int i = 0; i < count; i++) {
        move_entity(&asteroids[i], delta_time, ASTEROID);
    }
}

void init_asteroids(Entity *asteroids, int count) {
    init_rotation_tables();

    // Load asteroid model once and share it
    if (shared_asteroid_model == NULL) {
        shared_asteroid_model = t3d_model_load("rom:/asteroid7.t3dm");
    }

    for (int i = 0; i < count; i++) {
        asteroids[i] = create_entity_shared(shared_asteroid_model, (T3DVec3){{0, 10, 0}},
                                      randomize_float(0.1f, 1.3f), COLOR_ASTEROID, DRAW_SHADED, 10.0f);
        reset_entity(&asteroids[i], ASTEROID);
    }
}

// =============================================================================
// Resource Functions
// =============================================================================

void update_resources(Entity *resources, int count, float delta_time) {
    for (int i = 0; i < count; i++) {
        move_entity(&resources[i], delta_time, RESOURCE);
    }
}

void init_resources(Entity *resources, int count) {
    init_rotation_tables();

    // Use same shared model as asteroids (load if not already loaded)
    if (shared_asteroid_model == NULL) {
        shared_asteroid_model = t3d_model_load("rom:/asteroid7.t3dm");
    }

    for (int i = 0; i < count; i++) {
        resources[i] = create_entity_shared(shared_asteroid_model, (T3DVec3){{0, 10, 0}},
                                      1.0f, COLOR_RESOURCE, DRAW_SHADED, 10.0f);
        reset_entity(&resources[i], RESOURCE);
        resources[i].position.v[0] = randomize_float(-RESOURCE_BOUND_X, RESOURCE_BOUND_X);
        resources[i].position.v[2] = randomize_float(-RESOURCE_BOUND_Z, RESOURCE_BOUND_Z);
    }
}

// =============================================================================
// Optimized Asteroid System (matrix pool + tight struct)
// =============================================================================
// Matrix pool - only allocate matrices for visible asteroids
static T3DMat4FP *asteroid_matrix_pool = NULL;
static bool matrix_pool_used[ASTEROID_MATRIX_POOL_SIZE];

void init_asteroid_system(void) {
    // Load shared model
    if (shared_asteroid_model == NULL) {
        shared_asteroid_model = t3d_model_load("rom:/asteroid7.t3dm");
    }

    // Allocate matrix pool (uncached memory for RCP)
    if (asteroid_matrix_pool == NULL) {
        asteroid_matrix_pool = malloc_uncached(sizeof(T3DMat4FP) * ASTEROID_MATRIX_POOL_SIZE);
    }

    // Clear pool usage
    for (int i = 0; i < ASTEROID_MATRIX_POOL_SIZE; i++) {
        matrix_pool_used[i] = false;
    }
}

void reset_asteroid(Asteroid *asteroid) {
    // Get difficulty-scaled speed
    float difficulty = get_asteroid_speed_for_difficulty();
    float min_speed = ASTEROID_BASE_MIN_SPEED * difficulty;
    float max_speed = ASTEROID_BASE_MAX_SPEED * difficulty;

    asteroid->speed = randomize_float(min_speed, max_speed);
    asteroid->scale = randomize_float(ASTEROID_MIN_SCALE, ASTEROID_MAX_SCALE);
    asteroid->matrix_index = -1;  // No matrix assigned

    float bound_radius = ASTEROID_BOUND_X;

    // Spawn on circle edge at random angle
    float angle = randomize_float(0.0f, TWO_PI);
    asteroid->position.v[0] = cosf(angle) * bound_radius;
    asteroid->position.v[2] = sinf(angle) * bound_radius;

    // Velocity points toward center with some randomness
    float target_angle = angle + T3D_PI + randomize_float(-0.5f, 0.5f);
    asteroid->velocity.v[0] = cosf(target_angle);
    asteroid->velocity.v[2] = sinf(target_angle);

    // Normalize velocity
    float len_sq = asteroid->velocity.v[0] * asteroid->velocity.v[0] +
                   asteroid->velocity.v[2] * asteroid->velocity.v[2];
    if (len_sq > 0.0001f) {
        float inv_len = fast_inv_sqrt(len_sq);
        asteroid->velocity.v[0] *= inv_len;
        asteroid->velocity.v[2] *= inv_len;
    }

    asteroid->position.v[1] = 10.0f;  // Fixed Y height
    asteroid->velocity.v[1] = 0.0f;
    asteroid->rotation_y = randomize_float(0.0f, 360.0f);
}

void init_asteroids_optimized(Asteroid *asteroids, int count) {
    init_asteroid_system();

    for (int i = 0; i < count; i++) {
        reset_asteroid(&asteroids[i]);
    }
}

void update_asteroids_optimized(Asteroid *asteroids, int count, float delta_time) {
    float bound_radius = ASTEROID_BOUND_X + ASTEROID_PADDING;
    float bound_radius_sq = bound_radius * bound_radius;

    for (int i = 0; i < count; i++) {
        Asteroid *a = &asteroids[i];

        float move_amount = a->speed * delta_time;
        a->position.v[0] += a->velocity.v[0] * move_amount;
        a->position.v[2] += a->velocity.v[2] * move_amount;

        // Rotation only for nearby asteroids
        float dx = a->position.v[0] - game.cursor_position.v[0];
        float dz = a->position.v[2] - game.cursor_position.v[2];
        float dist_sq = dx * dx + dz * dz;

        if (dist_sq < ASTEROID_ROTATE_DISTANCE_SQ) {
            a->rotation_y += move_amount * 0.03f;
            if (a->rotation_y >= 360.0f) a->rotation_y -= 360.0f;
            if (a->rotation_y < 0.0f) a->rotation_y += 360.0f;
        }

        // Boundary check (circular)
        float pos_dist_sq = a->position.v[0] * a->position.v[0] +
                            a->position.v[2] * a->position.v[2];
        if (pos_dist_sq > bound_radius_sq) {
            reset_asteroid(a);
        }
    }
}

// Allocate a matrix from the pool, returns index or -1 if full
static int8_t allocate_matrix(void) {
    for (int i = 0; i < ASTEROID_MATRIX_POOL_SIZE; i++) {
        if (!matrix_pool_used[i]) {
            matrix_pool_used[i] = true;
            return (int8_t)i;
        }
    }
    return -1;  // Pool full
}

// Release all matrices back to pool (call each frame before drawing)
static void release_all_matrices(void) {
    for (int i = 0; i < ASTEROID_MATRIX_POOL_SIZE; i++) {
        matrix_pool_used[i] = false;
    }
}

void draw_asteroids_optimized(Asteroid *asteroids, bool *visibility, float *distance_sq, int count) {
    // Release all matrices from last frame
    release_all_matrices();

    // Build sorted list of visible asteroid indices (closest first)
    static struct { int index; float dist; } sorted[ASTEROID_MATRIX_POOL_SIZE];
    int visible_count = 0;

    for (int i = 0; i < count; i++) {
        if (!visibility[i]) continue;

        // Insert into sorted list (simple insertion sort, limited to pool size)
        if (visible_count < ASTEROID_MATRIX_POOL_SIZE) {
            // Find insertion point
            int insert_at = visible_count;
            for (int j = 0; j < visible_count; j++) {
                if (distance_sq[i] < sorted[j].dist) {
                    insert_at = j;
                    break;
                }
            }
            // Shift elements
            for (int j = visible_count; j > insert_at; j--) {
                sorted[j] = sorted[j-1];
            }
            sorted[insert_at].index = i;
            sorted[insert_at].dist = distance_sq[i];
            visible_count++;
        } else if (distance_sq[i] < sorted[ASTEROID_MATRIX_POOL_SIZE-1].dist) {
            // Closer than furthest in list - replace it
            int insert_at = ASTEROID_MATRIX_POOL_SIZE - 1;
            for (int j = 0; j < ASTEROID_MATRIX_POOL_SIZE - 1; j++) {
                if (distance_sq[i] < sorted[j].dist) {
                    insert_at = j;
                    break;
                }
            }
            // Shift elements (drop the last one)
            for (int j = ASTEROID_MATRIX_POOL_SIZE - 1; j > insert_at; j--) {
                sorted[j] = sorted[j-1];
            }
            sorted[insert_at].index = i;
            sorted[insert_at].dist = distance_sq[i];
        }
    }

    // Set up shared rendering state
    rdpq_set_prim_color(COLOR_ASTEROID);
    rdpq_mode_combiner(RDPQ_COMBINER1((PRIM, 0, SHADE, 0), (PRIM, 0, SHADE, 0)));

    // Draw sorted asteroids (closest first)
    for (int s = 0; s < visible_count; s++) {
        int i = sorted[s].index;
        Asteroid *a = &asteroids[i];

        // Allocate matrix from pool
        int8_t mat_idx = allocate_matrix();
        if (mat_idx < 0) break;  // Pool exhausted

        a->matrix_index = mat_idx;
        T3DMat4FP *matrix = &asteroid_matrix_pool[mat_idx];

        t3d_mat4fp_from_srt_euler(matrix,
            (float[3]){a->scale, a->scale, a->scale},
            (float[3]){0.0f, a->rotation_y, 0.0f},
            a->position.v);

        // Draw
        t3d_matrix_push(matrix);
        t3d_model_draw(shared_asteroid_model);
        t3d_matrix_pop(1);
    }
}

void free_asteroid_system(void) {
    if (asteroid_matrix_pool != NULL) {
        free_uncached(asteroid_matrix_pool);
        asteroid_matrix_pool = NULL;
    }
}

// =============================================================================
// Cleanup
// =============================================================================

void free_shared_models(void) {
    if (shared_asteroid_model != NULL) {
        t3d_model_free(shared_asteroid_model);
        shared_asteroid_model = NULL;
    }
    free_asteroid_system();
}