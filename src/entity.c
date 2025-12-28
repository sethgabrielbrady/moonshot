#include "entity.h"
#include <rdpq.h>

Entity create_entity(const char *model_path, T3DVec3 position, float scale, color_t color, DrawType draw_type, float collision_radius) {
    Entity entity = {
        .model = t3d_model_load(model_path),
        .matrix = malloc_uncached(sizeof(T3DMat4FP)),
        .position = position,
        .velocity = {{0.0f, 0.0f, 0.0f}},
        .scale = scale,
        .color = color,
        .rotation = {{0.0f, 0.0f, 0.0f}},
        // .rotation_x = 0.0f,
        // .rotation_y = 0.0f,
        // .rotation_z = 0.0f,
        .speed = 1.0f,
        .collision_radius = collision_radius,
        .value = RESOURCE_VALUE,
        .draw_type = draw_type
    };
    return entity;
}

void update_entity_matrix(Entity *entity) {
    float scale[3] = {entity->scale, entity->scale, entity->scale};
     //float rotation[3] = {entity->rotation_x, entity->rotation_y, entity->rotation_z};

    float rotation[3] = {entity->rotation.v[0], entity->rotation.v[1], entity->rotation.v[2]};
    t3d_mat4fp_from_srt_euler(entity->matrix, scale, rotation, entity->position.v);
}

void update_entity_matrices(Entity *entity_array, int count) {
    for (int i = 0; i < count; i++) {
        update_entity_matrix(&entity_array[i]);
    }
}

void rotate_entity(Entity *entity, float delta_time, float speed) {
    // entity->rotation_y += delta_time * speed;
    // if (entity->rotation_y > TWO_PI) {
    //     entity->rotation_y -= TWO_PI;
    // }
    entity->rotation.v[1] += delta_time * speed;
    if (entity->rotation.v[1] > TWO_PI) {
        entity->rotation.v[1] -= TWO_PI;
    }
}

void draw_entity(Entity *entity) {
    t3d_matrix_push(entity->matrix);
    rdpq_set_prim_color(entity->color);

    if (entity->draw_type == DRAW_SHADED) {
        rdpq_mode_combiner(RDPQ_COMBINER1((PRIM, 0, SHADE, 0), (PRIM, 0, SHADE, 0)));
    } else {
        rdpq_mode_combiner(RDPQ_COMBINER1((TEX0, 0, SHADE, 0), (TEX0, 0, SHADE, 0)));
    }

    t3d_model_draw(entity->model);
    t3d_matrix_pop(1);
}

void draw_entities(Entity *entity_array, int count) {
    for (int i = 0; i < count; i++) {
        draw_entity(&entity_array[i]);
    }
}

bool check_entity_intersection(Entity *a, Entity *b) {
    float combined_radius = a->collision_radius + b->collision_radius;

    // Skip if either has no collision
    if (combined_radius <= 0) return false;

    float dx = a->position.v[0] - b->position.v[0];
    float dz = a->position.v[2] - b->position.v[2];

    float distance_sq = dx * dx + dz * dz;
    return distance_sq < (combined_radius * combined_radius);
}

void free_entity(Entity *entity) {
    if (entity->model) {
        t3d_model_free(entity->model);
        entity->model = NULL;
    }
    if (entity->matrix) {
        free_uncached(entity->matrix);
        entity->matrix = NULL;
    }
}

void free_all_entities(Entity *entity_array, int count) {
    for (int i = 0; i < count; i++) {
        free_entity(&entity_array[i]);
    }
}