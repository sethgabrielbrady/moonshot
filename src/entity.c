#include "entity.h"
#include "camera.h"
#include "utils.h"
#include <rdpq.h>
#include <math.h>

// =============================================================================
// Entity Creation
// =============================================================================

Entity create_entity(const char *model_path, T3DVec3 position, float scale,
                     color_t color, DrawType draw_type, float collision_radius) {
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

// Create entity with shared model (no model loading, reuses existing)
Entity create_entity_shared(T3DModel *shared_model, T3DVec3 position, float scale,
                            color_t color, DrawType draw_type, float collision_radius) {
    Entity entity = {
        .model = shared_model,
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

// =============================================================================
// Entity Matrix Updates
// =============================================================================

void update_entity_matrix(Entity *entity) {
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

// =============================================================================
// Entity Rotation
// =============================================================================

void rotate_entity(Entity *entity, float delta_time, float speed) {
    entity->rotation.v[1] += delta_time * speed;
    if (entity->rotation.v[1] > TWO_PI) {
        entity->rotation.v[1] -= TWO_PI;
    }
}

// =============================================================================
// Entity Drawing
// =============================================================================

void draw_entity(Entity *entity) {
    draw_entity_with_fade(entity, 0.0f);
}

void draw_entity_with_fade(Entity *entity, float fade_distance) {
    t3d_matrix_push(entity->matrix);

    color_t render_color = entity->color;

    // Distance-based opacity fading
    if (fade_distance > 0.0f) {
        float dx = camera.position.v[0] - entity->position.v[0];
        float dy = camera.position.v[1] - entity->position.v[1];
        float dz = camera.position.v[2] - entity->position.v[2];
        float distance_sq = dx * dx + dy * dy + dz * dz;

        // Use fast inverse sqrt to get distance: dist = dist_sq * (1/sqrt(dist_sq))
        float distance = distance_sq * fast_inv_sqrt(distance_sq);

        float min_distance = 50.0f;
        float max_distance = fade_distance;

        float blend_factor = (distance - min_distance) / (max_distance - min_distance);
        if (blend_factor < 0.0f) blend_factor = 0.0f;
        if (blend_factor > 1.0f) blend_factor = 1.0f;

        uint32_t color_val = color_to_packed32(entity->color);
        uint8_t r = (color_val >> 24) & 0xFF;
        uint8_t g = (color_val >> 16) & 0xFF;
        uint8_t b = (color_val >> 8) & 0xFF;
        uint8_t original_a = color_val & 0xFF;

        uint8_t a = (uint8_t)(30 + blend_factor * (original_a - 30));

        render_color = RGBA32(r, g, b, a);
    }

    rdpq_set_prim_color(render_color);

    if (entity->draw_type == DRAW_FLAT) {
        rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
    } else if (entity->draw_type == DRAW_SHADED) {
        rdpq_mode_combiner(RDPQ_COMBINER1((PRIM, 0, SHADE, 0), (PRIM, 0, SHADE, 0)));
    } else {
        // Textured lit - texture with vertex shading
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

// =============================================================================
// Entity Collision
// =============================================================================

bool check_entity_intersection(Entity *a, Entity *b) {
    if (a->collision_radius <= 0.0f || b->collision_radius <= 0.0f) return false;

    float combined_radius = a->collision_radius + b->collision_radius;
    float combined_radius_sq = combined_radius * combined_radius;

    float dx = a->position.v[0] - b->position.v[0];
    float dz = a->position.v[2] - b->position.v[2];

    float distance_sq = dx * dx + dz * dz;
    return distance_sq < combined_radius_sq;
}

// =============================================================================
// Entity Cleanup
// =============================================================================

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

// Free entity that uses a shared model (don't free the model)
void free_entity_shared(Entity *entity) {
    entity->model = NULL;  // Don't free shared model
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

void free_all_entities_shared(Entity *entity_array, int count) {
    for (int i = 0; i < count; i++) {
        free_entity_shared(&entity_array[i]);
    }
}