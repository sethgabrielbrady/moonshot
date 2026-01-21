#ifndef SPAWNER_H
#define SPAWNER_H

#include "types.h"

// =============================================================================
// Entity Reset/Spawning (for Entity-based objects)
// =============================================================================

void reset_entity(Entity *entity, EntityType type);
void move_entity(Entity *entity, float delta_time, EntityType type);

// =============================================================================
// Optimized Asteroid Functions (uses Asteroid struct + matrix pool)
// =============================================================================

void init_asteroid_system(void);
void init_asteroids_optimized(Asteroid *asteroids, int count);
void update_asteroids_optimized(Asteroid *asteroids, int count, float delta_time);
void reset_asteroid(Asteroid *asteroid);
void draw_asteroids_optimized(Asteroid *asteroids, bool *visibility, float *distance_sq, int count);
void free_asteroid_system(void);

// =============================================================================
// Legacy Asteroid Functions (for Entity-based asteroids)
// =============================================================================

void init_asteroids(Entity *asteroids, int count);
void update_asteroids(Entity *asteroids, int count, float delta_time);

// =============================================================================
// Resource Functions
// =============================================================================

void init_resources(Entity *resources, int count);
void update_resources(Entity *resources, int count, float delta_time);
void scale_resource_based_on_value(Entity *resource);

// =============================================================================
// Cleanup
// =============================================================================

void free_shared_models(void);

#endif // SPAWNER_H