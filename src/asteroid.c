#include "asteroid.h"
#include "constants.h"
#include "utils.h"
#include "entity.h"
#include <math.h>


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

    float len = sqrtf(entity->velocity.v[0] * entity->velocity.v[0] +
                      entity->velocity.v[2] * entity->velocity.v[2]);
    if (len > 0) {
        entity->velocity.v[0] /= len;
        entity->velocity.v[2] /= len;
    }
}


void move_entity(Entity *entity, float delta_time, EntityType type) {
    entity->velocity.v[0] += randomize_float(-0.1f, 0.1f) * delta_time;
    entity->velocity.v[2] += randomize_float(-0.1f, 0.1f) * delta_time;

    float len = sqrtf(entity->velocity.v[0] * entity->velocity.v[0] +
                      entity->velocity.v[2] * entity->velocity.v[2]);
    if (len > 0.1f) {
        entity->velocity.v[0] /= len;
        entity->velocity.v[2] /= len;
    }

    float move_amount = entity->speed * delta_time;
    entity->position.v[0] += entity->velocity.v[0] * move_amount;
    entity->position.v[2] += entity->velocity.v[2] * move_amount;

    // entity->rotation_x += delta_time * entity->speed * 0.05f;
    // entity->rotation_y += delta_time * entity->speed * 0.03f;
    // entity->rotation_z += delta_time * entity->speed * 0.02f;
    entity->rotation.v[0] += delta_time * entity->speed * 0.05f;
    entity->rotation.v[1] += delta_time * entity->speed * 0.03f;
    entity->rotation.v[2] += delta_time * entity->speed * 0.02f;

    bool out_of_bounds =
        entity->position.v[0] < -(ASTEROID_BOUND_X + ASTEROID_PADDING) ||
        entity->position.v[0] > (ASTEROID_BOUND_X + ASTEROID_PADDING) ||
        entity->position.v[2] < -(ASTEROID_BOUND_Z + ASTEROID_PADDING) ||
        entity->position.v[2] > (ASTEROID_BOUND_Z + ASTEROID_PADDING);

    if (out_of_bounds && type == ASTEROID ) {
       reset_entity(entity, ASTEROID);
    }
    if (out_of_bounds && type == RESOURCE ) {
       reset_entity(entity, RESOURCE);
    }
}



//consolidate
void update_asteroids(Entity *asteroids, int count, float delta_time) {
    for (int i = 0; i < count; i++) {
        move_entity(&asteroids[i], delta_time, ASTEROID);
    }
}
void update_resources(Entity *resources, int count, float delta_time) {
    for (int i = 0; i < count; i++) {
        // scale_resource_based_on_value(&resources[i]);
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
