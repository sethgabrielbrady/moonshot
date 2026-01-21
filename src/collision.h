#ifndef COLLISION_H
#define COLLISION_H

#include "types.h"

// =============================================================================
// Color Flash System
// =============================================================================

void start_entity_color_flash(Entity *entity, color_t flash_color, float duration_seconds);
void update_color_flashes(float delta_time);

// =============================================================================
// Cursor/Ship Collisions (Optimized Asteroid struct)
// =============================================================================

void check_cursor_asteroid_collisions_opt(Entity *cursor, Asteroid *asteroids, int count, bool *asteroid_visible, float delta_time);
void check_cursor_asteroid_deflection_opt(Entity *cursor, Asteroid *asteroids, int count);

// =============================================================================
// Cursor/Ship Collisions (Legacy Entity struct)
// =============================================================================

void check_cursor_asteroid_collisions(Entity *cursor, Entity *asteroids, int count, bool *asteroid_visible, float delta_time);
void check_cursor_station_collision(Entity *cursor, Entity *station);
void check_cursor_resource_collisions(Entity *cursor, Entity *resources, int count, float delta_time);
void check_cursor_asteroid_deflection(Entity *cursor, Entity *asteroids, int count);

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

float calculate_asteroid_damage_opt(Asteroid *asteroid);
float calculate_asteroid_damage(Entity *asteroid);
void reset_resource_colors(Entity *resources, int count);

void update_deflect_timer(float delta_time);
void check_deflect_input(void);

// Deflection radius for visualization
#define DEFLECT_RADIUS 15.0f
#define DEFLECT_DURATION 0.40f  // 400ms window

#endif // COLLISION_H