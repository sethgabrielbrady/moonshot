#include "entity.h"
#include "camera.h"
#include "utils.h"
#include <rdpq.h>
#include <math.h>

Entity create_entity(const char *model_path, T3DVec3 position, float scale, color_t color, DrawType draw_type, float collision_radius) {
    Entity entity = {
        .model = t3d_model_load(model_path),
        .matrix = malloc_uncached(sizeof(T3DMat4FP)),
        .position = position,
        .velocity = {{0.0f, 0.0f, 0.0f}},
        .scale = scale,
        .color = color,
        .rotation = {{0.0f, 0.0f, 0.0f}},
        .speed = 1.0f,
        .collision_radius = collision_radius,
        .value = 0,
        .draw_type = draw_type,
    };
    return entity;
}

void update_entity_matrix(Entity *entity) {
    // Use compound literals to avoid temporary array allocations
    t3d_mat4fp_from_srt_euler(entity->matrix,
        (float[3]){entity->scale, entity->scale, entity->scale},
        (float[3]){entity->rotation.v[0], entity->rotation.v[1], entity->rotation.v[2]},
        entity->position.v);
}

void update_entity_matrices(Entity *entity_array, int count) {
    for (int i = 0; i < count; i++) {
        update_entity_matrix(&entity_array[i]);
    }
}

void rotate_entity(Entity *entity, float delta_time, float speed) {
    entity->rotation.v[1] += delta_time * speed;
    // Normalize to [0, TWO_PI) range
    if (entity->rotation.v[1] > TWO_PI) {
        entity->rotation.v[1] -= TWO_PI;
    }
}

void draw_entity(Entity *entity) {
    draw_entity_with_fade(entity, 0.0f);
}

void draw_entity_with_fade(Entity *entity, float fade_distance) {
    t3d_matrix_push(entity->matrix);

    color_t render_color = entity->color;

    // Apply distance-based opacity fading if fade_distance > 0
    if (fade_distance > 0.0f) {
        float dx = camera.position.v[0] - entity->position.v[0];
        float dy = camera.position.v[1] - entity->position.v[1];
        float dz = camera.position.v[2] - entity->position.v[2];
        float distance = sqrtf(dx * dx + dy * dy + dz * dz);

        // Closer = more transparent, farther = more opaque
        float min_distance = 50.0f;   // Fully transparent at this distance
        float max_distance = fade_distance; // Fully opaque beyond this

        float blend_factor = (distance - min_distance) / (max_distance - min_distance);
        if (blend_factor < 0.0f) blend_factor = 0.0f;
        if (blend_factor > 1.0f) blend_factor = 1.0f;

        // Extract original RGBA
        uint32_t color_val = color_to_packed32(entity->color);
        uint8_t r = (color_val >> 24) & 0xFF;
        uint8_t g = (color_val >> 16) & 0xFF;
        uint8_t b = (color_val >> 8) & 0xFF;
        uint8_t original_a = color_val & 0xFF;

        // Interpolate alpha from 30 (near-transparent) to original_a (full opacity)
        uint8_t a = (uint8_t)(30 + blend_factor * (original_a - 30));

        render_color = RGBA32(r, g, b, a);
    }

    rdpq_set_prim_color(render_color);

    // Set combiner mode based on draw type
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
    // Early exit if either has no collision radius
    if (a->collision_radius <= 0.0f || b->collision_radius <= 0.0f) return false;

    float combined_radius = a->collision_radius + b->collision_radius;
    float combined_radius_sq = combined_radius * combined_radius;

    float dx = a->position.v[0] - b->position.v[0];
    float dz = a->position.v[2] - b->position.v[2];

    float distance_sq = dx * dx + dz * dz;
    return distance_sq < combined_radius_sq;
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