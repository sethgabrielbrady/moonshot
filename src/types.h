#ifndef TYPES_H
#define TYPES_H

#include <libdragon.h>
#include <t3d/t3d.h>
#include <t3d/t3dmath.h>
#include <t3d/t3dmodel.h>

typedef enum {
    DRAW_SHADED,
    DRAW_TEXTURED_LIT
} DrawType;

typedef struct {
    T3DVec3 position;
    T3DVec3 target;
    float rotation_y;
} Camera;

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


typedef enum {
    ENTITY_STATION,
    ENTITY_CURSOR,
    ENTITY_DRONE,
    ENTITY_TILE,
    ENTITY_GRID,
    ENTITY_COUNT
} EntityID;

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
    // ASTEROID_A21,
    // ASTEROID_A22,
    // ASTEROID_A23,
    // ASTEROID_A24,
    // ASTEROID_A25,
    // ASTEROID_A26,
    // ASTEROID_A27,
    // ASTEROID_A28,
    // ASTEROID_A29,
    // ASTEROID_A30,
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

typedef enum {
    R1,
    R2,
    R3,
    R4,
    R5,
    R6,
    RESOURCE_COUNT
} ResourceID;

typedef enum {
    RESOURCE_STATE_DEFAULT,
    RESOURCE_STATE_HIGHLIGHTED,
    RESOURCE_STATE_MINING,
    RESOURCE_STATE_DEPLETED
} ResourceState;

typedef enum {
    ASTEROID,
    RESOURCE
} EntityType;

typedef struct {
    Entity *entity;
    color_t original_color;
    color_t flash_color;
    float timer;
    float duration;
    bool active;
} ColorFlash;

typedef struct {
  T3DVec3 pos;
  float strength;
  color_t color;
} PointLight;


typedef struct {
    T3DVec3 position;
    T3DVec3 velocity;
    color_t color;
    float size;
    float lifetime;
    float max_lifetime;
    bool active;
} ParticleData;

// resolution_t hi_res = {
//   .width = 640,
//   .height = 480,
//   .interlaced = true
// };

// resolution_t lo_res = {
//   .width = 320,
//   .height = 240,
//   .interlaced = false
// };


#endif
