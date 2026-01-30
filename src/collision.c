#include "collision.h"
#include "constants.h"
#include "game_state.h"
#include "entity.h"
#include "particles.h"
#include "audio.h"
#include "camera.h"
#include "spawner.h"
#include <math.h>

// =============================================================================
// External References (from main.c - will be moved to game_state later)
// =============================================================================

// These need to be passed in or accessed via game state
// For now, declare as extern until full migration
extern bool *asteroid_visible;
extern bool *resource_visible;
extern Entity *cursor_entity;
extern T3DVec3 cursor_velocity;

// =============================================================================
// Color Flash System
// =============================================================================

static ColorFlash color_flashes[MAX_COLOR_FLASHES];

void update_color_flashes(float delta_time) {
    for (int i = 0; i < MAX_COLOR_FLASHES; i++) {
        if (!color_flashes[i].active) continue;

        color_flashes[i].timer += delta_time;

        if (color_flashes[i].timer >= color_flashes[i].duration) {
            color_flashes[i].entity->color = color_flashes[i].original_color;
            color_flashes[i].active = false;
            color_flashes[i].entity = NULL;
        } else {
            int blink_state = (int)(color_flashes[i].timer * 10) % 2;
            if (blink_state == 0) {
                color_flashes[i].entity->color = color_flashes[i].flash_color;
            } else {
                color_flashes[i].entity->color = color_flashes[i].original_color;
            }
        }
    }
}

// =============================================================================
// Damage Calculation
// =============================================================================

float calculate_asteroid_damage(Entity *asteroid) {
    // Mass approximation (volume scales with cube of size)
    float mass = asteroid->scale * asteroid->scale * asteroid->scale;
    float kinetic_energy = 0.25f * mass * asteroid->speed * asteroid->speed;
    return kinetic_energy * DAMAGE_MULTIPLIER;
}

// =============================================================================
// Station Collisions
// =============================================================================


void check_loader_asteroid_collisions_opt(Entity *loader, Asteroid *asteroids, int count, float delta_time) {
    const float asteroid_collision_radius = 10.0f;

    for (int i = 0; i < count; i++) {
        // Distance-based early rejection
        float dx = loader->position.v[0] - asteroids[i].position.v[0];
        float dz = loader->position.v[2] - asteroids[i].position.v[2];
        float dist_sq = dx * dx + dz * dz;
        float max_range = loader->collision_radius + asteroid_collision_radius + 50.0f;
        if (dist_sq > max_range * max_range) continue;

        // Actual collision check
        float combined_radius = loader->collision_radius + asteroid_collision_radius;
        if (dist_sq < combined_radius * combined_radius) {
            spawn_explosion(asteroids[i].position, COLOR_SPARKS);
            // play_sfx(4);
            reset_asteroid(&asteroids[i]);
        }
    }
}

// =============================================================================
// Cursor/Ship Collisions
// =============================================================================

static float ship_damage_multiplier = 3.0f;

void check_cursor_asteroid_collisions(Entity *cursor, Entity *asteroids, int count, bool *visibility, float delta_time) {
    if (game.deflect_active) return;
    if (game.cursor_iframe_timer > 0.0f) {
        game.cursor_iframe_timer -= delta_time;
    }

    for (int i = 0; i < count; i++) {
        // Skip culled asteroids
        if (visibility && !visibility[i]) continue;

        // Distance-based early rejection
        float dx = cursor->position.v[0] - asteroids[i].position.v[0];
        float dz = cursor->position.v[2] - asteroids[i].position.v[2];
        float dist_sq = dx * dx + dz * dz;
        float max_range = cursor->collision_radius + asteroids[i].collision_radius + 30.0f;
        if (dist_sq > max_range * max_range) continue;

        if (check_entity_intersection(cursor, &asteroids[i])) {

            if (game.cursor_iframe_timer <= 0.0f) {
                play_sfx(4);
                float damage = calculate_asteroid_damage(&asteroids[i]);
                if (damage <= MAX_DAMAGE * ship_damage_multiplier) {
                    damage = MAX_DAMAGE * ship_damage_multiplier;
                }
                // if (damage >= MAX_DAMAGE * ship_damage_multiplier) {
                //     damage = MAX_DAMAGE * ship_damage_multiplier;
                // }
                spawn_explosion(asteroids[i].position, COLOR_SPARKS);
                cursor->value -= damage;
                game.cursor_last_damage = (int)damage;

                if (cursor->value < 0) {
                    cursor->value = 0;
                    game.disabled_controls = true;
                } else if (cursor->value > 0 && game.ship_fuel > 0) {
                    game.disabled_controls = false;
                }

                // Add asteroid velocity to cursor velocity (knockback)
                game.cursor_velocity.v[0] += asteroids[i].velocity.v[0] * KNOCKBACK_STRENGTH;
                game.cursor_velocity.v[2] += asteroids[i].velocity.v[2] * KNOCKBACK_STRENGTH;

                other_shake_enabled = damage > 0 ? true : false;
                if (other_shake_enabled) {
                    trigger_screen_shake(3.0f, 0.25f);
                }
                game.cursor_iframe_timer = CURSOR_IFRAME_DURATION;
            }
            reset_entity(&asteroids[i], ASTEROID);

        }
    }
}


int stored_cursor_resource_val = 0;
float value_multiplier = 0.3f;

void check_cursor_station_collision(Entity *cursor, Entity *station) {
    if (!check_entity_intersection(cursor, station)) return;

    // Only process if we have resources to deposit
    if (game.cursor_resource_val > 0) {
        game.hauled_resources = true;
    }


    stored_cursor_resource_val = game.cursor_resource_val;

    // Health restoration
    if (cursor->value < CURSOR_MAX_HEALTH) {
        if (stored_cursor_resource_val >= 100) {
            cursor->value += 50;
        } else {
            cursor->value += stored_cursor_resource_val * value_multiplier;
        }
        if (cursor->value > CURSOR_MAX_HEALTH) {
            cursor->value = CURSOR_MAX_HEALTH;
        }
    }

    // Fuel restoration
    if (game.ship_fuel < CURSOR_MAX_FUEL) {
        if (stored_cursor_resource_val >= 70) {
            game.ship_fuel = CURSOR_MAX_FUEL;
        } else {
            game.ship_fuel += stored_cursor_resource_val * 0.85f;
        }
        if (game.ship_fuel > CURSOR_MAX_FUEL) {
            game.ship_fuel = CURSOR_MAX_FUEL;
        }
    }

    // Credits - bonus for full load
    if (stored_cursor_resource_val >= 100) {
        game.accumulated_credits += stored_cursor_resource_val + 25;
    } else {
        game.accumulated_credits += stored_cursor_resource_val;
    }
    // Clear resources
    game.cursor_resource_val = 0;
    stored_cursor_resource_val = 0;
}


void check_cursor_asteroid_deflection(Entity *cursor, Entity *asteroids, int count) {

    if (!game.deflect_active) return;

    // Check for A button press to start deflection window
    joypad_buttons_t pressed = joypad_get_buttons_pressed(JOYPAD_PORT_1);
    if (pressed.b && !game.deflect_active) {
        game.deflect_active = true;
        game.deflect_timer = DEFLECT_DURATION;
    }


    // Only deflect if window is active
    if (!game.deflect_active) return;

    // is this right?
    float deflect_radius_sq = DEFLECT_RADIUS * DEFLECT_RADIUS;

    for (int i = 0; i < count; i++) {
        float dx = cursor->position.v[0] - asteroids[i].position.v[0];
        float dz = cursor->position.v[2] - asteroids[i].position.v[2];
        float dist_sq = dx * dx + dz * dz;

        if (dist_sq < deflect_radius_sq) {
            spawn_explosion(asteroids[i].position, COLOR_ASTEROID);
            play_sfx(SFX_SHIP_HIT);
            reset_entity(&asteroids[i], ASTEROID);
            game.deflect_count++;
        }
    }
}


void update_deflect_timer(float delta_time) {
    if (game.deflect_active) {
        game.deflect_timer -= delta_time;
        if (game.deflect_timer <= 0.0f) {
            game.deflect_active = false;
            game.deflect_timer = 0.0f;
        }
    }
}

// =============================================================================
// Mining Functions
// =============================================================================

static float mining_accumulated = 0.0f;
static bool cursor_was_mining = false;

static void mine_resource(Entity *entity, Entity *resource, float delta_time) {
    float amount = delta_time * MINING_RATE;
    mining_accumulated += amount;

    // Only transfer whole units
    if (mining_accumulated >= 1.0f) {
        int transfer = (int)mining_accumulated;
        resource->value -= transfer;
        game.cursor_resource_val += transfer;
        mining_accumulated -= transfer;
        spawn_mining_sparks(resource->position);
    }

    game.resource_val = resource->value;

    if (resource->value <= 0) {
        mining_accumulated = 0.0f;
        resource->value = 0;
        resource->color = COLOR_ASTEROID;
        entity->value += 5; // replenish some energy on resource depletion
        reset_entity(resource, RESOURCE);
    }
}

void check_cursor_resource_collisions(Entity *cursor, Entity *resources, int count, float delta_time) {
    game.cursor_mining_resource = -1;
    game.cursor_is_mining = false;
    bool found_mining = false;

    for (int i = 0; i < count; i++) {
        // Distance-based early rejection
        float dx = cursor->position.v[0] - resources[i].position.v[0];
        float dz = cursor->position.v[2] - resources[i].position.v[2];
        float dist_sq = dx * dx + dz * dz;
        float max_range = cursor->collision_radius + resources[i].collision_radius + 20.0f;
        if (dist_sq > max_range * max_range) {
            cursor->color = COLOR_CURSOR;
            continue;
        }

        if (check_entity_intersection(cursor, &resources[i]) &&
            (game.cursor_resource_val < CURSOR_RESOURCE_CAPACITY)) {
            game.cursor_is_mining = true;

            // Flicker both resource and cursor color - welder effect
            int flicker = rand() % 100;
            if (flicker < 10) {
                cursor->color = RGBA32(0, 0, 0, 255);
            } else if (flicker < 40) {
                resources[i].color = RGBA32(255, 255, 255, 255);
                cursor->color = RGBA32(100, 100, 100, 255);
            } else if (flicker < 70) {
                resources[i].color = RGBA32(200, 200, 200, 255);
                cursor->color = RGBA32(220, 220, 220, 255);
            } else {
                resources[i].color = COLOR_RESOURCE;
                cursor->color = COLOR_CURSOR;
            }

            // Only play sound when mining starts
            if (!cursor_was_mining) {
                play_sfx(1);
            }
            found_mining = true;
            game.cursor_mining_resource = i;
            mine_resource(cursor, &resources[i], delta_time);
            break;
        } else {
            cursor->color = COLOR_CURSOR;
        }
    }

    cursor_was_mining = found_mining;
}

// =============================================================================
// Drone Collisions
// =============================================================================

static void drone_mine_resource(Entity *entity, Entity *resource, float delta_time) {
    float amount = delta_time * DRONE_MINING_RATE;
    game.drone_mining_accumulated += amount;

    // Only transfer whole units
    if (game.drone_mining_accumulated >= 1.0f && game.drone_resource_val < DRONE_MAX_RESOURCES) {
        int transfer = (int)game.drone_mining_accumulated;
        resource->value -= transfer;
        game.drone_resource_val += transfer;
        entity->value = game.drone_resource_val;
        game.drone_mining_accumulated -= transfer;
        spawn_mining_sparks(entity->position);
    }

    game.resource_val = resource->value;

    if (resource->value <= 0) {
        game.drone_mining_accumulated = 0.0f;
        resource->value = 0;
        resource->color = COLOR_ASTEROID;
        reset_entity(resource, RESOURCE);
    }
}

void check_drone_resource_collisions(Entity *entity, Entity *resources, int count, float delta_time) {
    game.drone_mining_resource = -1;
    game.drone_collecting_resource = false;
    game.drone_is_mining = false;

    // Update drone_full based on current resource level
    game.drone_full = (game.drone_resource_val >= DRONE_MAX_RESOURCES);

    // Don't mine if drone was just given a new command
    // if (game.drone_command_interrupt) {
    //     return;
    // }

    for (int i = 0; i < count; i++) {
        // Distance-based early rejection
        float dx = entity->position.v[0] - resources[i].position.v[0];
        float dz = entity->position.v[2] - resources[i].position.v[2];
        float dist_sq = dx * dx + dz * dz;
        float max_range = entity->collision_radius + resources[i].collision_radius + 20.0f;
        if (dist_sq > max_range * max_range) continue;

        if (check_entity_intersection(entity, &resources[i]) &&
            game.drone_resource_val < DRONE_MAX_RESOURCES) {
            game.drone_collecting_resource = true;
            game.drone_is_mining = true;
            entity->position.v[1] = resources[i].position.v[1] + 5.0f;
            entity->position.v[0] = resources[i].position.v[0];
            entity->position.v[2] = resources[i].position.v[2];

            // Flicker resource color - welder effect
            int flicker = rand() % 100;
            if (flicker < 40) {
                resources[i].color = RGBA32(111, 239, 255, 255);
            } else if (flicker < 70) {
                resources[i].color = RGBA32(200, 200, 200, 255);
            } else {
                resources[i].color = COLOR_RESOURCE;
            }

            game.drone_mining_resource = i;
            drone_mine_resource(entity, &resources[i], delta_time);

            break;
        }
    }
}

void check_drone_station_collisions(Entity *drone, Entity *station, int count) {
    if (check_entity_intersection(drone, station)) {
        // station->value += game.drone_resource_val;
        game.accumulated_credits += game.drone_resource_val;
        game.drone_resource_val = 0;
        drone->value = game.drone_resource_val;
    }
}

void check_drone_cursor_collisions(Entity *drone, Entity *cursor, int count) {
    if (check_entity_intersection(drone, cursor)) {
        cursor->value += game.drone_resource_val;

        game.ship_fuel += game.drone_resource_val * 0.3f;

        if (game.ship_fuel < 10) {
            game.ship_fuel = 10;
        }
        if (cursor->value > CURSOR_MAX_HEALTH) {
            cursor->value = CURSOR_MAX_HEALTH;
        }
        if (game.ship_fuel > CURSOR_MAX_FUEL) {
            game.ship_fuel = CURSOR_MAX_FUEL;
        }

        game.drone_resource_val = 0;
        drone->value = game.drone_resource_val;
        if (cursor->value > 0) {
            game.disabled_controls = false;
        }

        // End heal mode after successfully healing
        game.drone_heal = false;
        game.move_drone = false;
    }
}

// =============================================================================
// Tile Collisions
// =============================================================================

void check_tile_resource_collision(Entity *tile, Entity *resources, int count) {
    // Only check if tile is visible and not already following something
    if (tile->position.v[1] > 100.0f) return;
    if (game.drone_full) return;

    for (int i = 0; i < count; i++) {
        if (check_entity_intersection(tile, &resources[i])) {
            // Start following this resource
            game.tile_following_resource = i;

            // Send drone to this resource
            game.drone_target_position.v[0] = resources[i].position.v[0];
            game.drone_target_position.v[1] = resources[i].position.v[1];
            game.drone_target_position.v[2] = resources[i].position.v[2];
            game.drone_target_rotation = 0.0f;
            game.move_drone = true;
            game.drone_moving_to_resource = true;
            game.drone_moving_to_station = false;

            return;
        }
    }
}

// =============================================================================
// Utility Functions
// =============================================================================

void reset_resource_colors(Entity *resources, int count) {
    for (int i = 0; i < count; i++) {
        if (i == game.tile_following_resource) {
            resources[i].color = COLOR_TILE;
        } else if (i != game.highlighted_resource &&
                   i != game.cursor_mining_resource &&
                   i != game.drone_mining_resource) {
            resources[i].color = COLOR_RESOURCE;
        } else if (resources[i].value <= 0) {
            resources[i].color = COLOR_ASTEROID;
        }
    }
}

void check_deflect_input(void) {
    // Check for A button press to start deflection window (called every frame)
    joypad_buttons_t pressed = joypad_get_buttons_pressed(JOYPAD_PORT_1);
    if (game.ship_fuel >= DEFLECT_FUEL_COST) {
        if (pressed.b && !game.deflect_active) {
            game.deflect_active = true;
            game.deflect_timer = DEFLECT_DURATION;
            game.ship_fuel -= DEFLECT_FUEL_COST;
        }
    }

}

// =============================================================================
// Optimized Asteroid Collision Functions (uses Asteroid struct)
// =============================================================================

float calculate_asteroid_damage_opt(Asteroid *asteroid) {
    float mass = asteroid->scale * asteroid->scale * asteroid->scale;
    float kinetic_energy = 0.25f * mass * asteroid->speed * asteroid->speed;
    return kinetic_energy * DAMAGE_MULTIPLIER;
}

void check_cursor_asteroid_collisions_opt(Entity *cursor, Asteroid *asteroids, int count, bool *visibility, float delta_time) {
    if (game.deflect_active) return;
    if (game.cursor_iframe_timer > 0.0f) {
        game.cursor_iframe_timer -= delta_time;
    }

    // Asteroid collision radius (constant for all)
    const float asteroid_collision_radius = 10.0f;

    for (int i = 0; i < count; i++) {
        if (visibility && !visibility[i]) continue;

        // Distance-based early rejection
        float dx = cursor->position.v[0] - asteroids[i].position.v[0];
        float dz = cursor->position.v[2] - asteroids[i].position.v[2];
        float dist_sq = dx * dx + dz * dz;
        float max_range = cursor->collision_radius + asteroid_collision_radius + 30.0f;
        if (dist_sq > max_range * max_range) continue;

        // Actual collision check
        float combined_radius = cursor->collision_radius + asteroid_collision_radius;
        if (dist_sq < combined_radius * combined_radius) {
            if (game.cursor_iframe_timer <= 0.0f) {
                play_sfx(4);
                float damage = calculate_asteroid_damage_opt(&asteroids[i]);
                if (damage <= MAX_DAMAGE * ship_damage_multiplier) {
                    damage = MAX_DAMAGE * ship_damage_multiplier;
                }
                spawn_explosion(asteroids[i].position, COLOR_SPARKS);
                cursor->value -= damage;
                game.cursor_last_damage = (int)damage;

                if (cursor->value < 0) {
                    cursor->value = 0;
                    game.disabled_controls = true;
                } else if (cursor->value > 0 && game.ship_fuel > 0) {
                    game.disabled_controls = false;
                }

                // Add asteroid velocity to cursor velocity (knockback)
                game.cursor_velocity.v[0] += asteroids[i].velocity.v[0] * KNOCKBACK_STRENGTH;
                game.cursor_velocity.v[2] += asteroids[i].velocity.v[2] * KNOCKBACK_STRENGTH;

                other_shake_enabled = damage > 0 ? true : false;
                if (other_shake_enabled) {
                    trigger_screen_shake(3.0f, 0.25f);
                }
                game.cursor_iframe_timer = CURSOR_IFRAME_DURATION;
            }
            reset_asteroid(&asteroids[i]);
        }
    }
}

void check_cursor_asteroid_deflection_opt(Entity *cursor, Asteroid *asteroids, int count) {
    if (!game.deflect_active) return;

    float deflect_radius_sq = DEFLECT_RADIUS * DEFLECT_RADIUS;

    for (int i = 0; i < count; i++) {
        float dx = cursor->position.v[0] - asteroids[i].position.v[0];
        float dz = cursor->position.v[2] - asteroids[i].position.v[2];
        float dist_sq = dx * dx + dz * dz;

        if (dist_sq < deflect_radius_sq) {
            spawn_explosion(asteroids[i].position, COLOR_ASTEROID);
            play_sfx(SFX_SHIP_HIT);
            reset_asteroid(&asteroids[i]);
            game.deflect_count++;
        }
    }
}

// =============================================================================
// Loader Asteroid Collisions (Optimized Asteroid struct)
// =============================================================================
