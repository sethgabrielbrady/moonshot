#ifndef PARTICLES_H
#define PARTICLES_H

#include <t3d/t3d.h>
#include <libdragon.h>

// =============================================================================
// Initialization / Cleanup
// =============================================================================

void init_particles(void);
void cleanup_particles(void);
void clear_all_particles(void);

// =============================================================================
// Particle Spawning
// =============================================================================

void spawn_explosion(T3DVec3 position, color_t color);
void spawn_mining_sparks(T3DVec3 position);
void spawn_loader_sparks(T3DVec3 position);
void spawn_ship_trail(T3DVec3 position, T3DVec3 velocity, color_t color);

// =============================================================================
// Update / Draw
// =============================================================================

void update_particles(float delta_time);
void draw_particles(T3DViewport *viewport);

// =============================================================================
// Ambient Particles
// =============================================================================

void init_ambient_particles(void);

// =============================================================================
// Debug
// =============================================================================

extern int debug_particle_count;

#endif // PARTICLES_H