#ifndef COLLISION_H
#define COLLISION_H

#include "types.h"

// =============================================================================
// Color Flash System
// =============================================================================

void start_entity_color_flash(Entity *entity, color_t flash_color, float duration_seconds);
void update_color_flashes(float delta_time);

// =============================================================================
// Station Collisions
// =============================================================================

void check_station_asteroid_collisions(Entity *station, Entity *asteroids, int count, float delta_time);

// =============================================================================
// Cursor/Ship Collisions
// =============================================================================

void check_cursor_asteroid_collisions(Entity *cursor, Entity *asteroids, int count, bool *asteroid_visible);
void check_cursor_station_collision(Entity *cursor, Entity *station);
void check_cursor_resource_collisions(Entity *cursor, Entity *resources, int count, float delta_time);

// =============================================================================
// Drone Collisions
// =============================================================================

void check_drone_resource_collisions(Entity *drone, Entity *resources, int count, float delta_time);
void check_drone_station_collisions(Entity *drone, Entity *station, int count);
void check_drone_cursor_collisions(Entity *drone, Entity *cursor, int count);

// =============================================================================
// Tile Collisions
// =============================================================================

void check_tile_resource_collision(Entity *tile, Entity *resources, int count);

// =============================================================================
// Utility
// =============================================================================

float calculate_asteroid_damage(Entity *asteroid);
void reset_resource_colors(Entity *resources, int count);

#endif // COLLISION_H
