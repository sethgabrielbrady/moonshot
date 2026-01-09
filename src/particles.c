#include "particles.h"
#include <t3d/t3d.h>
#include <t3d/tpx.h>
#include <stdlib.h>
#include <math.h>
#include "types.h"
#include "constants.h"
#include "camera.h"

// =============================================================================
// Configuration
// =============================================================================

#define MAX_PARTICLES 512
#define MAX_AMBIENT_PARTICLES 400



// =============================================================================
// Debug
// =============================================================================

int debug_particle_count = 0;


// =============================================================================
// Ambient Particle Structure
// =============================================================================

typedef struct {
    T3DVec3 position;
    float size;
    bool active;
} AmbientParticle;

// =============================================================================
// Static Data
// =============================================================================

static AmbientParticle ambient_particles[MAX_AMBIENT_PARTICLES];
static ParticleData particle_data[MAX_PARTICLES];
static TPXParticle *tpx_particles = NULL;
static sprite_t *particle_sprite = NULL;
static T3DMat4FP *particle_matrix = NULL;

// =============================================================================
// Initialization
// =============================================================================

void init_particles(void) {
    particle_sprite = sprite_load("rom:/particle.sprite");
    tpx_particles = malloc_uncached(sizeof(TPXParticle) * MAX_PARTICLES);
    particle_matrix = malloc_uncached(sizeof(T3DMat4FP));

    tpx_init((TPXInitParams){});

    for (int i = 0; i < MAX_PARTICLES; i++) {
        particle_data[i].active = false;
    }
}

void cleanup_particles(void) {
    if (particle_sprite) {
        sprite_free(particle_sprite);
        particle_sprite = NULL;
    }
    if (tpx_particles) {
        free_uncached(tpx_particles);
        tpx_particles = NULL;
    }
    if (particle_matrix) {
        free_uncached(particle_matrix);
        particle_matrix = NULL;
    }
}

void clear_all_particles(void) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        particle_data[i].active = false;
    }
}

// =============================================================================
// Particle Spawning
// =============================================================================

static int get_free_particle(void) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!particle_data[i].active) {
            return i;
        }
    }
    return -1;
}

static void spawn_particle(T3DVec3 position, T3DVec3 velocity, color_t color, float size, float lifetime) {
    int idx = get_free_particle();
    if (idx < 0) return;

    ParticleData *p = &particle_data[idx];
    p->position = position;
    p->velocity = velocity;
    p->color = color;
    p->size = size * 1.0f;
    p->lifetime = lifetime;
    p->max_lifetime = lifetime;
    p->active = true;
}

void spawn_explosion(T3DVec3 position) {
    for (int i = 0; i < 16; i++) {
        T3DVec3 velocity = {{
            (rand() % 250 - 100) * 1.0f,
            (rand() % 80 + 20) * 1.0f,
            (rand() % 250 - 100) * 1.0f
        }};
        spawn_particle(position, velocity, COLOR_SPARKS, 0.04f, 0.4f);
    }
}

void spawn_mining_sparks(T3DVec3 position) {
    for (int i = 0; i < 4; i++) {
        T3DVec3 velocity = {{
            (rand() % 200 - 100) * 0.75f,
            (rand() % 90 + 20) * 1.0f,
            (rand() % 200 - 100) * 0.75f
        }};
        spawn_particle(position, velocity, COLOR_RESOURCE, 0.03f, 0.6f);
    }
}

void spawn_exhaust(T3DVec3 position, T3DVec3 direction) {
    for (int i = 0; i < 2; i++) {
        T3DVec3 velocity = {{
            direction.v[0] * -30.0f + (rand() % 20 - 10),
            (rand() % 20) * 0.25f,
            direction.v[2] * -30.0f + (rand() % 20 - 10)
        }};
        color_t color = RGBA32(255, 248, 4, 125);
        spawn_particle(position, velocity, color, 0.02f, 0.3f);
    }
}

void spawn_station_explosion(T3DVec3 position) {
    clear_all_particles();
    trigger_screen_shake(8.0f, 1.0f);
    trigger_camera_zoom(position, 3.0f);

    // Primary explosion
    for (int i = 0; i < 80; i++) {
        float angle_h = (rand() % 360) * (3.14159f / 180.0f);
        float angle_v = (rand() % 180 - 90) * (3.14159f / 180.0f);
        float speed = 150.0f + (rand() % 200);

        T3DVec3 velocity = {{
            cosf(angle_h) * cosf(angle_v) * speed,
            sinf(angle_v) * speed + 80.0f,
            sinf(angle_h) * cosf(angle_v) * speed
        }};

        color_t color;
        int color_type = rand() % 3;
        switch (color_type) {
            case 0: color = RGBA32(255, 220, 50, 255); break;
            case 1: color = RGBA32(255, 120, 0, 255); break;
            case 2: color = RGBA32(255, 50, 50, 255); break;
        }

        float size = 0.10f + (rand() % 80) * 0.001f;
        float lifetime = 2.0f + (rand() % 150) * 0.01f;

        spawn_particle(position, velocity, color, size, lifetime);
    }

    // Secondary ring burst
    for (int i = 0; i < 40; i++) {
        float angle = (rand() % 360) * (3.14159f / 180.0f);
        float speed = 200.0f + (rand() % 150);

        T3DVec3 velocity = {{
            cosf(angle) * speed,
            20.0f + (rand() % 60),
            sinf(angle) * speed
        }};

        color_t color = RGBA32(255, 180, 50, 255);
        float size = 0.08f + (rand() % 60) * 0.001f;
        float lifetime = 1.5f + (rand() % 100) * 0.01f;

        spawn_particle(position, velocity, color, size, lifetime);
    }

    // Debris particles
    for (int i = 0; i < 30; i++) {
        T3DVec3 velocity = {{
            (rand() % 400 - 200) * 0.5f,
            (rand() % 250) * 0.5f,
            (rand() % 400 - 200) * 0.5f
        }};

        color_t color = RGBA32(180, 180, 180, 255);
        float size = 0.04f + (rand() % 30) * 0.001f;
        float lifetime = 2.5f + (rand() % 150) * 0.01f;

        spawn_particle(position, velocity, color, size, lifetime);
    }
}

// =============================================================================
// Particle Update
// =============================================================================

void update_particles(float delta_time) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        ParticleData *p = &particle_data[i];
        if (!p->active) continue;

        p->lifetime -= delta_time;
        if (p->lifetime <= 0) {
            p->active = false;
            continue;
        }

        // Gravity and drag
        p->velocity.v[1] -= 150.0f * delta_time;

        float drag = 0.98f;
        p->velocity.v[0] *= drag;
        p->velocity.v[1] *= 0.99f;
        p->velocity.v[2] *= drag;

        // Position update
        p->position.v[0] += p->velocity.v[0] * delta_time;
        p->position.v[1] += p->velocity.v[1] * delta_time;
        p->position.v[2] += p->velocity.v[2] * delta_time;
    }
}

// =============================================================================
// Particle Drawing
// =============================================================================

void draw_particles(T3DViewport *viewport) {
    if (!particle_sprite || !tpx_particles) return;

    int active_count = 0;
    TPXParticle *tpx = tpx_particles;

    // Regular particles
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!particle_data[i].active) continue;

        ParticleData *p = &particle_data[i];

        // Frustum culling
        float dx = p->position.v[0] - camera.position.v[0];
        float dz = p->position.v[2] - camera.position.v[2];
        float dist_sq = dx * dx + dz * dz;
        if (dist_sq > 1000000.0f) continue;

        float life_ratio = p->lifetime / p->max_lifetime;
        uint8_t alpha = (uint8_t)(life_ratio * 255);
        float size = p->size * (0.5f + life_ratio * 0.5f);
        int8_t size_fp = (int8_t)(size * 64);

        // Position relative to camera
        float rel_x = p->position.v[0] - camera.position.v[0];
        float rel_y = p->position.v[1] - camera.position.v[1];
        float rel_z = p->position.v[2] - camera.position.v[2];

        int8_t px = (int8_t)(rel_x * 0.1f);
        int8_t py = (int8_t)(rel_y * 0.1f);
        int8_t pz = (int8_t)(rel_z * 0.1f);

        tpx->posA[0] = tpx->posB[0] = px;
        tpx->posA[1] = tpx->posB[1] = py;
        tpx->posA[2] = tpx->posB[2] = pz;
        tpx->sizeA = tpx->sizeB = size_fp;

        uint8_t r = p->color.r, g = p->color.g, b = p->color.b;
        tpx->colorA[0] = tpx->colorB[0] = r;
        tpx->colorA[1] = tpx->colorB[1] = g;
        tpx->colorA[2] = tpx->colorB[2] = b;
        tpx->colorA[3] = tpx->colorB[3] = alpha;

        tpx++;
        active_count++;
    }

    // Ambient particles
    for (int i = 0; i < MAX_AMBIENT_PARTICLES; i++) {
        AmbientParticle *ap = &ambient_particles[i];
        if (!ap->active) continue;

        float dx = ap->position.v[0] - camera.position.v[0];
        float dz = ap->position.v[2] - camera.position.v[2];
        float dist_sq = dx * dx + dz * dz;
        if (dist_sq > 1500000.0f) continue;

        int8_t size_fp = (int8_t)(ap->size * 64);

        float rel_x = ap->position.v[0] - camera.position.v[0];
        float rel_y = ap->position.v[1] - camera.position.v[1];
        float rel_z = ap->position.v[2] - camera.position.v[2];

        int8_t px = (int8_t)(rel_x * 0.1f);
        int8_t py = (int8_t)(rel_y * 0.1f);
        int8_t pz = (int8_t)(rel_z * 0.1f);

        tpx->posA[0] = tpx->posB[0] = px;
        tpx->posA[1] = tpx->posB[1] = py;
        tpx->posA[2] = tpx->posB[2] = pz;
        tpx->sizeA = tpx->sizeB = size_fp;

        tpx->colorA[0] = tpx->colorB[0] = 255;
        tpx->colorA[1] = tpx->colorB[1] = 248;
        tpx->colorA[2] = tpx->colorB[2] = 5;
        tpx->colorA[3] = tpx->colorB[3] = 255;

        tpx++;
        active_count++;
    }

    debug_particle_count = active_count;

    // TPX requires even count
    if (active_count & 1) {
        active_count--;
    }

    if (active_count < 2) return;

    // Matrix at camera position with 10x scale
    t3d_mat4fp_from_srt_euler(particle_matrix,
        (float[]){10.0f, 10.0f, 10.0f},
        (float[]){0, 0, 0},
        (float[]){camera.position.v[0], camera.position.v[1], camera.position.v[2]}
    );

    // Render state
    rdpq_sync_pipe();
    rdpq_sync_tile();
    rdpq_sync_load();
    rdpq_set_mode_standard();
    rdpq_mode_zbuf(true, true);

    rdpq_mode_zoverride(true, 0, 0);
    rdpq_mode_filter(FILTER_POINT);
    rdpq_mode_alphacompare(10);
    rdpq_mode_combiner(RDPQ_COMBINER1((PRIM, 0, TEX0, 0), (0, 0, 0, TEX0)));

    rdpq_texparms_t tex_params = {
        .s.repeats = REPEAT_INFINITE,
        .t.repeats = REPEAT_INFINITE,
        .s.scale_log = 0,
        .t.scale_log = 0,
    };
    rdpq_sprite_upload(TILE0, particle_sprite, &tex_params);

    tpx_state_from_t3d();
    tpx_matrix_push(particle_matrix);
    tpx_state_set_scale(1.0f, 1.0f);
    tpx_state_set_tex_params(0, 0);

    tpx_particle_draw_tex(tpx_particles, active_count);

    tpx_matrix_pop(1);

    rdpq_sync_pipe();
    rdpq_sync_tile();
}

// =============================================================================
// Ambient Particles
// =============================================================================
void init_ambient_particles(void) {
    for (int i = 0; i < MAX_AMBIENT_PARTICLES; i++) {
        AmbientParticle *p = &ambient_particles[i];
        p->active = true;

        // Random position in play area
        p->position.v[0] = (rand() % ((int)PLAY_AREA_SIZE * 2)) - PLAY_AREA_SIZE;
        p->position.v[1] = 24.0f + (rand() % 77);
        p->position.v[2] = (rand() % ((int)PLAY_AREA_SIZE * 2)) - PLAY_AREA_SIZE;

        p->size = (10 + (rand() % 31)) * 0.001f;
    }
}