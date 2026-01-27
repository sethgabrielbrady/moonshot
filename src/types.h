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
    DRAW_TEXTURED_LIT,
    DRAW_FLAT,
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
    ENTITY_LOADER,
    ENTITY_LOADER_VERT,
    ENTITY_DEFLECT_RING,
    ENTITY_GRID,
    ENTITY_WALL,
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
    ASTEROID_A26,
    ASTEROID_A27,
    ASTEROID_A28,
    ASTEROID_A29,
    ASTEROID_A30,
    // ASTEROID_A31,
    // ASTEROID_A32,
    // ASTEROID_A33,
    // ASTEROID_A34,
    // ASTEROID_A35,
    // ASTEROID_A36,
    // ASTEROID_A37,
    // ASTEROID_A38,
    // ASTEROID_A39,
    // ASTEROID_A40,
    // ASTEROID_A41,
    // ASTEROID_A42,
    // ASTEROID_A43,
    // ASTEROID_A44,
    // ASTEROID_A45,
    // ASTEROID_A46,
    // ASTEROID_A47,
    // ASTEROID_A48,
    // ASTEROID_A49,
    // ASTEROID_A50,
    ASTEROID_COUNT
} AsteroidID;

// =============================================================================
// Optimized Asteroid Structure (32 bytes - fits in 2 cache lines)
// Shared data (model, color, collision_radius) stored separately
// =============================================================================

typedef struct {
    T3DVec3 position;         // 12 bytes
    T3DVec3 velocity;         // 12 bytes
    float scale;              // 4 bytes
    float rotation_y;         // 4 bytes (only need Y rotation)
    float speed;              // 4 bytes
    int8_t matrix_index;      // 1 byte (-1 = no matrix assigned)
    uint8_t padding[3];       // 3 bytes padding for alignment
} Asteroid;                   // Total: 40 bytes

// Maximum visible asteroids at once (matrix pool size)
#define ASTEROID_MATRIX_POOL_SIZE 48

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
    R7,
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