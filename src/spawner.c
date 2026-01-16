#include "spawner.h"
#include "constants.h"
#include "utils.h"
#include "entity.h"
#include <math.h>
#include <stdint.h>

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

static float asteroid_speed_multiplier = 200.0f;
static float asteroid_min_scale = 0.3f;
static float asteroid_max_scale = 1.3f;

static void get_asteroid_velocity_and_scale(Entity *asteroid, T3DVec3 *out_velocity) {
    asteroid->speed = randomize_float(asteroid_speed_multiplier, 450.0f); // what is this number?
    asteroid->scale = randomize_float(asteroid_min_scale, asteroid_max_scale);
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
    int edge = rand() % 4;

    if (type == ASTEROID) {
        get_asteroid_velocity_and_scale(entity, &entity->velocity);
    }
    if (type == RESOURCE) {
        get_resource_velocity_and_scale(entity, &entity->velocity);
        entity->color = COLOR_RESOURCE;
        scale_resource_based_on_value(entity);
    }

    float bound_x = (type == RESOURCE) ? RESOURCE_BOUND_X : ASTEROID_BOUND_X;
    float bound_z = (type == RESOURCE) ? RESOURCE_BOUND_Z : ASTEROID_BOUND_Z;

    switch (edge) {
        case 0:
            entity->position.v[0] = randomize_float(-bound_x, bound_x);
            entity->position.v[2] = -bound_z;
            entity->velocity.v[0] = randomize_float(-0.5f, 0.5f);
            entity->velocity.v[2] = randomize_float(0.3f, 1.0f);
            break;
        case 1:
            entity->position.v[0] = randomize_float(-bound_x, bound_x);
            entity->position.v[2] = bound_z;
            entity->velocity.v[0] = randomize_float(-0.5f, 0.5f);
            entity->velocity.v[2] = randomize_float(-1.0f, -0.3f);
            break;
        case 2:
            entity->position.v[0] = bound_x;
            entity->position.v[2] = randomize_float(-bound_z, bound_z);
            entity->velocity.v[0] = randomize_float(-1.0f, -0.3f);
            entity->velocity.v[2] = randomize_float(-0.5f, 0.5f);
            break;
        case 3:
            entity->position.v[0] = -bound_x;
            entity->position.v[2] = randomize_float(-bound_z, bound_z);
            entity->velocity.v[0] = randomize_float(0.3f, 1.0f);
            entity->velocity.v[2] = randomize_float(-0.5f, 0.5f);
            break;
    }

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

    // Y rotation
    entity->rotation.v[1] += move_amount * 0.03f;
    while (entity->rotation.v[1] >= 360.0f) entity->rotation.v[1] -= 360.0f;
    while (entity->rotation.v[1] < 0.0f) entity->rotation.v[1] += 360.0f;

    // Boundary check
    float bound_x = ASTEROID_BOUND_X + ASTEROID_PADDING;
    float bound_z = ASTEROID_BOUND_Z + ASTEROID_PADDING;

    if (entity->position.v[0] < -bound_x || entity->position.v[0] > bound_x ||
        entity->position.v[2] < -bound_z || entity->position.v[2] > bound_z) {
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

    for (int i = 0; i < count; i++) {
        asteroids[i] = create_entity("rom:/asteroid1.t3dm", (T3DVec3){{0, 10, 0}},
                                      randomize_float(0.1f, 1.3f), COLOR_ASTEROID, DRAW_SHADED, 10.0f);
        reset_entity(&asteroids[i], ASTEROID);
        // Scatter across field initially
        asteroids[i].position.v[0] = randomize_float(-ASTEROID_BOUND_X, ASTEROID_BOUND_X);
        asteroids[i].position.v[2] = randomize_float(-ASTEROID_BOUND_Z, ASTEROID_BOUND_Z);
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

    for (int i = 0; i < count; i++) {
        resources[i] = create_entity("rom:/asteroid1.t3dm", (T3DVec3){{0, 10, 0}},
                                      1.0f, COLOR_RESOURCE, DRAW_SHADED, 10.0f);
        reset_entity(&resources[i], RESOURCE);
        resources[i].position.v[0] = randomize_float(-RESOURCE_BOUND_X, RESOURCE_BOUND_X);
        resources[i].position.v[2] = randomize_float(-RESOURCE_BOUND_Z, RESOURCE_BOUND_Z);
    }
}
