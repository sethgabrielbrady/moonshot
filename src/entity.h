#ifndef ENTITY_H
#define ENTITY_H

#include "types.h"
#include "constants.h"

Entity create_entity(const char *model_path, T3DVec3 position, float scale, color_t color, DrawType draw_type, float collision_radius);
void update_entity_matrix(Entity *entity);
void update_entity_matrices(Entity *entity_array, int count);
void rotate_entity(Entity *entity, float delta_time, float speed);
void draw_entity(Entity *entity);
void draw_entity_with_fade(Entity *entity, float fade_distance);
void draw_entities(Entity *entity_array, int count);
void free_entity(Entity *entity);
void free_all_entities(Entity *entity_array, int count);
bool check_entity_intersection(Entity *a, Entity *b);

#endif
