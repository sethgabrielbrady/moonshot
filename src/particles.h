#ifndef PARTICLES_H
#define PARTICLES_H

#include <libdragon.h>
#include <t3d/t3d.h>
#include <t3d/tpx.h>


void draw_particle_display(T3DVec3 position);
void init_particles(void);
void update_particles(float delta_time);
void draw_particles(T3DViewport *viewport);
void spawn_explosion(T3DVec3 position);
void spawn_mining_sparks(T3DVec3 position);
void spawn_station_explosion(T3DVec3 position);  // New
void clear_all_particles(void);
void spawn_exhaust(T3DVec3 position, T3DVec3 direction);
void cleanup_particles(void);

// Ambient particles
void init_ambient_particles(void);
void update_ambient_particles(float delta_time);

extern int debug_particle_count;


#endif