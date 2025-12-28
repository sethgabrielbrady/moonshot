#ifndef ASTEROID_H
#define ASTEROID_H

#include "types.h"

void reset_entity(Entity *entity, EntityType type);
void move_entity(Entity *entity, float delta_time,  EntityType type);
void update_asteroids(Entity *asteroids, int count, float delta_time);
void update_resources(Entity *resources, int count, float delta_time);

void init_asteroids(Entity *asteroids, int count);
void init_resources(Entity *resources, int count);



#endif