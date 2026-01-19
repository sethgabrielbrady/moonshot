#ifndef TYPES_H
#define TYPES_H

#include <libdragon.h>
#include <t3d/t3d.h>
#include <t3d/t3dmath.h>
#include <t3d/t3dmodel.h>

// =============================================================================
// Draw Types
// =============================================================================

typedef enum {
    DRAW_SHADED,
    DRAW_TEXTURED_LIT
} DrawType;

// =============================================================================
// Entity Structure
// =============================================================================

typedef struct {
    T3DModel *model;
    T3DMat4FP *matrix;
    T3DVec3 position;
    T3DVec3 velocity;
    color_t color;
    float scale;
    T3DVec3 rotation;
    float speed;
    float collision_radius;
    int value;
    DrawType draw_type;
} Entity;

// =============================================================================
// Entity IDs
// =============================================================================

typedef enum {
    ENTITY_STATION,
    ENTITY_CURSOR,
    ENTITY_DRONE,
    ENTITY_TILE,
    ENTITY_DEFLECT_RING,
    ENTITY_GRID,
    ENTITY_COUNT
} EntityID;

// =============================================================================
// Asteroid IDs
// =============================================================================

typedef enum {
    ASTEROID_A1,
    ASTEROID_A2,
    ASTEROID_A3,
    ASTEROID_A4,
    ASTEROID_A5,
    ASTEROID_A6,
    ASTEROID_A7,
    ASTEROID_A8,
    ASTEROID_A9,
    ASTEROID_A10,
    ASTEROID_A11,
    ASTEROID_A12,
    ASTEROID_A13,
    ASTEROID_A14,
    ASTEROID_A15,
    ASTEROID_A16,
    ASTEROID_A17,
    ASTEROID_A18,
    ASTEROID_A19,
    ASTEROID_A20,
    ASTEROID_A21,
    ASTEROID_A22,
    ASTEROID_A23,
    ASTEROID_A24,
    ASTEROID_A25,
    // ASTEROID_A26,
    // ASTEROID_A27,
    // ASTEROID_A28,
    // ASTEROID_A29,
    // ASTEROID_A30,
    ASTEROID_COUNT
} AsteroidID;

// =============================================================================
// Resource IDs
// =============================================================================

typedef enum {
    R1,
    R2,
    R3,
    R4,
    R5,
    R6,
    RESOURCE_COUNT
} ResourceID;

// =============================================================================
// Resource States
// =============================================================================

typedef enum {
    RESOURCE_STATE_DEFAULT,
    RESOURCE_STATE_HIGHLIGHTED,
    RESOURCE_STATE_MINING,
    RESOURCE_STATE_DEPLETED
} ResourceState;

// =============================================================================
// Entity Types (for spawning)
// =============================================================================

typedef enum {
    ASTEROID,
    RESOURCE
} EntityType;

// =============================================================================
// Color Flash Effect
// =============================================================================

typedef struct {
    Entity *entity;
    color_t original_color;
    color_t flash_color;
    float timer;
    float duration;
    bool active;
} ColorFlash;

// =============================================================================
// Point Light
// =============================================================================

typedef struct {
    T3DVec3 pos;
    float strength;
    color_t color;
} PointLight;

// =============================================================================
// Particle Data (used by particle system)
// =============================================================================

typedef struct {
    T3DVec3 position;
    T3DVec3 velocity;
    color_t color;
    float size;
    float lifetime;
    float max_lifetime;
    bool active;
} ParticleData;

#endif // TYPES_H
