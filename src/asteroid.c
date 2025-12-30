#include "asteroid.h"
#include "constants.h"
#include "utils.h"
#include "entity.h"
#include <math.h>
#include <stdint.h>

// Fast inverse square root for efficient normalization
static inline float fast_inv_sqrt(float x) {
    // Quake III algorithm with union to avoid strict-aliasing issues
    union {
        float f;
        int32_t i;
    } conv;

    float halfx = 0.5f * x;
    conv.f = x;
    conv.i = 0x5f3759df - (conv.i >> 1);
    conv.f = conv.f * (1.5f - (halfx * conv.f * conv.f));
    return conv.f;
}


void get_asteroid_velocity_and_scale (Entity *asteroid, T3DVec3 *out_velocity) {
    asteroid->speed = randomize_float(200.0f, 450.0f);
    asteroid->scale = randomize_float(0.1f, 1.3f);
}

void get_resource_velocity_and_scale (Entity *resource, T3DVec3 *out_velocity) {
    resource->speed = randomize_float(5.0f, 20.0f);
    resource->scale = randomize_float(0.6f, 1.4f);
    // resource->value = RESOURCE_VALUE
}

//function that updates the resource scale based on value
void scale_resource_based_on_value(Entity *resource) {
    resource->value = resource->scale * VALUE_MULTIPLIER;
}

void reset_entity(Entity *entity, EntityType type ) {
    int edge = rand() % 4;

    if (type == ASTEROID) {
        get_asteroid_velocity_and_scale(entity, &entity->velocity);
    }
    if (type == RESOURCE) {
        get_resource_velocity_and_scale(entity, &entity->velocity);
        entity->color = COLOR_RESOURCE;
        scale_resource_based_on_value(entity);

    }
    // asteroid->scale = randomize_float(0.1f, 1.3f);

    switch (edge) {
        case 0:
            entity->position.v[0] = randomize_float(-ASTEROID_BOUND_X, ASTEROID_BOUND_X);
            entity->position.v[2] = -ASTEROID_BOUND_Z;
            entity->velocity.v[0] = randomize_float(-0.5f, 0.5f);
            entity->velocity.v[2] = randomize_float(0.3f, 1.0f);
            break;
        case 1:
            entity->position.v[0] = randomize_float(-ASTEROID_BOUND_X, ASTEROID_BOUND_X);
            entity->position.v[2] = ASTEROID_BOUND_Z;
            entity->velocity.v[0] = randomize_float(-0.5f, 0.5f);
            entity->velocity.v[2] = randomize_float(-1.0f, -0.3f);
            break;
        case 2:
            entity->position.v[0] = ASTEROID_BOUND_X;
            entity->position.v[2] = randomize_float(-ASTEROID_BOUND_Z, ASTEROID_BOUND_Z);
            entity->velocity.v[0] = randomize_float(-1.0f, -0.3f);
            entity->velocity.v[2] = randomize_float(-0.5f, 0.5f);
            break;
        case 3:
            entity->position.v[0] = -ASTEROID_BOUND_X;
            entity->position.v[2] = randomize_float(-ASTEROID_BOUND_Z, ASTEROID_BOUND_Z);
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


void move_entity(Entity *entity, float delta_time, EntityType type) {
    // Removed per-frame randomization for performance
    // Velocity direction remains constant for smoother, faster movement

    // Normalize velocity only if needed using fast inverse sqrt
    float len_sq = entity->velocity.v[0] * entity->velocity.v[0] +
                   entity->velocity.v[2] * entity->velocity.v[2];
    if (len_sq > 1.01f) {  // Only normalize if significantly denormalized
        float inv_len = fast_inv_sqrt(len_sq);
        entity->velocity.v[0] *= inv_len;
        entity->velocity.v[2] *= inv_len;
    }

    // Cache frequently used value
    float move_amount = entity->speed * delta_time;
    entity->position.v[0] += entity->velocity.v[0] * move_amount;
    entity->position.v[2] += entity->velocity.v[2] * move_amount;

    // Update rotation with cached value
    entity->rotation.v[0] += move_amount * 0.05f;
    entity->rotation.v[1] += move_amount * 0.03f;
    entity->rotation.v[2] += move_amount * 0.02f;

    // Early exit boundary check with optimized pattern
    float bound_x = ASTEROID_BOUND_X + ASTEROID_PADDING;
    float bound_z = ASTEROID_BOUND_Z + ASTEROID_PADDING;

    if (entity->position.v[0] < -bound_x || entity->position.v[0] > bound_x ||
        entity->position.v[2] < -bound_z || entity->position.v[2] > bound_z) {
        reset_entity(entity, type);
    }
}



// Optimized update functions
void update_asteroids(Entity *asteroids, int count, float delta_time) {
    for (int i = 0; i < count; i++) {
        move_entity(&asteroids[i], delta_time, ASTEROID);
    }
}

void update_resources(Entity *resources, int count, float delta_time) {
    for (int i = 0; i < count; i++) {
        move_entity(&resources[i], delta_time, RESOURCE);
    }
}

void init_asteroids(Entity *asteroids, int count) {
    for (int i = 0; i < count; i++) {
        asteroids[i] = create_entity("rom:/asteroid2.t3dm", (T3DVec3){{0, 10, 0}},
                                      randomize_float(0.1f, 1.3f), COLOR_ASTEROID, DRAW_SHADED, 10.0f);
        // Set up velocity and speed first
       reset_entity(&asteroids[i], ASTEROID);
        // Then override position to scatter them across the field initially
        // (instead of all starting at edges)
        asteroids[i].position.v[0] = randomize_float(-ASTEROID_BOUND_X, ASTEROID_BOUND_X);
        asteroids[i].position.v[2] = randomize_float(-ASTEROID_BOUND_Z, ASTEROID_BOUND_Z);
    }
}


void init_resources(Entity *resources, int count) {
    for (int i = 0; i < count; i++) {
        resources[i] = create_entity("rom:/asteroid.t3dm", (T3DVec3){{0, 10, 0}},
                                      1.0f, COLOR_RESOURCE, DRAW_SHADED, 10.0f);
        reset_entity(&resources[i], RESOURCE);
        resources[i].position.v[0] = randomize_float(-RESOURCE_BOUND_X, RESOURCE_BOUND_X);
        resources[i].position.v[2] = randomize_float(-RESOURCE_BOUND_Z, RESOURCE_BOUND_Z);
    }
}
