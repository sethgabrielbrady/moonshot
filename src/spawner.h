#ifndef SPAWNER_H
#define SPAWNER_H

#include "types.h"

// =============================================================================
// Entity Reset/Spawning
// =============================================================================

void reset_entity(Entity *entity, EntityType type);
void move_entity(Entity *entity, float delta_time, EntityType type);

// =============================================================================
// Asteroid Functions
// =============================================================================

void init_asteroids(Entity *asteroids, int count);
void update_asteroids(Entity *asteroids, int count, float delta_time);

// =============================================================================
// Resource Functions
// =============================================================================

void init_resources(Entity *resources, int count);
void update_resources(Entity *resources, int count, float delta_time);
void scale_resource_based_on_value(Entity *resource);

#endif // SPAWNER_H