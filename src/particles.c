#include "particles.h"
#include <t3d/t3d.h>
#include <t3d/tpx.h>
#include <stdlib.h>
#include "types.h"
#include "constants.h"

#define MAX_PARTICLES 256

int debug_particle_count = 0;


// typedef struct {
//     T3DVec3 position;
//     T3DVec3 velocity;
//     color_t color;
//     float size;
//     float lifetime;
//     float max_lifetime;
//     bool active;
// } ParticleData;

static ParticleData particle_data[MAX_PARTICLES];
static TPXParticle *tpx_particles = NULL;
static sprite_t *particle_sprite = NULL;
static T3DMat4FP *particle_matrix = NULL;

void init_particles(void) {
    particle_sprite = sprite_load("rom:/particle.sprite");
    tpx_particles = malloc_uncached(sizeof(TPXParticle) * MAX_PARTICLES);
    particle_matrix = malloc_uncached(sizeof(T3DMat4FP));

    tpx_init((TPXInitParams){});

    for (int i = 0; i < MAX_PARTICLES; i++) {
        particle_data[i].active = false;
    }
}

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
    p->position = position; // Invert Z for N64 coord system
    p->velocity = velocity;
    p->color = color;
    p->size = size * 1.0f;  // Scale down for N64
    p->lifetime = lifetime;
    p->max_lifetime = lifetime;
    p->active = true;

    // debug_particle_count++;  // Track spawns
}

void spawn_explosion(T3DVec3 position) {
    for (int i = 0; i < 20; i++) {  // 20 is even
        T3DVec3 velocity = {{
            (rand() % 200 - 100) * 1.0f,
            (rand() % 150 + 50) * 1.0f,
            (rand() % 200 - 100) * 1.0f
        }};
        // color_t color = RGBA32(255, 150 + (rand() % 105), 50, 255);
        // float size = 0.010f + (rand() % 20) * 0.01f;


        // if fps_mode is enabled, make particles smaller
        spawn_particle(position, velocity, COLOR_CURSOR, 0.04f , 0.8f + (rand() % 5) * 0.1f);
    }
}

void spawn_mining_sparks(T3DVec3 position) {
    for (int i = 0; i < 6; i++) {  // Changed from 5 to 6 (even)
        T3DVec3 velocity = {{
            (rand() % 200 - 100) * 0.75f,
            // (rand() % 60 + 30) * 0.75f,
            (rand() % 90 + 20) * 1.0f,
            (rand() % 200 - 100) * 0.75f
        }};
        // color_t color = RGBA32(150 + (rand() % 100), 0, 150 + (rand() % 100), 255);
        spawn_particle(position, velocity, COLOR_RESOURCE , 0.03f, 0.6f);
    }
}

// void spawn_exhaust(T3DVec3 position, T3DVec3 direction) {
//     for (int i = 0; i < 2; i++) {  // 2 is even
//         T3DVec3 velocity = {{
//             direction.v[0] * -30.0f + (rand() % 20 - 10),
//             (rand() % 20) * 0.25f,
//             direction.v[2] * -30.0f + (rand() % 20 - 10)
//         }};
//         color_t color = RGBA32(255, 248, 4, 250);
//         spawn_particle(position, velocity, color, 0.02f, 0.3f);
//     }
// }
void update_particles(float delta_time) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!particle_data[i].active) continue;

        ParticleData *p = &particle_data[i];

        // Update lifetime
        p->lifetime -= delta_time;
        if (p->lifetime <= 0) {
            p->active = false;
            continue;
        }

        // Apply gravity
        p->velocity.v[1] -= 150.0f * delta_time;

        // Apply drag
        p->velocity.v[0] *= 0.98f;
        p->velocity.v[1] *= 0.99f;
        p->velocity.v[2] *= 0.98f;

        // Update position
        p->position.v[0] += p->velocity.v[0] * delta_time;
        p->position.v[1] += p->velocity.v[1] * delta_time;
        p->position.v[2] += p->velocity.v[2] * delta_time;
    }
}


void draw_particles(T3DViewport *viewport) {
    if (!particle_sprite || !tpx_particles) return;

    // Build TPX particle array from active particles
    int active_count = 0;
    TPXParticle *tpx = tpx_particles;

    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!particle_data[i].active) continue;

        ParticleData *p = &particle_data[i];
        float life_ratio = p->lifetime / p->max_lifetime;

        // Fade alpha based on lifetime
        uint8_t alpha = (uint8_t)(life_ratio * 255);

        // Scale size based on lifetime
        float size = p->size * (0.5f + life_ratio * 0.5f);
        int8_t size_fp = (int8_t)(size * 64);

        // Position - TPX uses int8_t for small offsets relative to the matrix
        // Scale world positions down to fit in int8_t range (-128 to 127)
        int8_t px = (int8_t)(p->position.v[0] * 0.1f);
        int8_t py = (int8_t)(p->position.v[1] * 0.1f);
        int8_t pz = (int8_t)(p->position.v[2] * 0.1f);

        // Fill both A and B at once
        tpx->posA[0] = tpx->posB[0] = px;
        tpx->posA[1] = tpx->posB[1] = py;
        tpx->posA[2] = tpx->posB[2] = pz;
        tpx->sizeA = tpx->sizeB = size_fp;

        tpx->colorA[0] = tpx->colorB[0] = p->color.r;
        tpx->colorA[1] = tpx->colorB[1] = p->color.g;
        tpx->colorA[2] = tpx->colorB[2] = p->color.b;
        tpx->colorA[3] = tpx->colorB[3] = alpha;

        tpx++;
        active_count++;
    }

    debug_particle_count = active_count;

    // TPX requires even number of particles
    if (active_count & 1) {
        active_count--;
    }

    if (active_count < 2) return;

    // Matrix with 10x scale to compensate for 0.1x scale of positions above
    t3d_mat4fp_from_srt_euler(particle_matrix,
        (float[]){10.0f, 10.0f, 10.0f},
        (float[]){0, 0, 0},
        (float[]){0, 0, 0}
    );

    // Set up rendering state
    rdpq_sync_pipe();
    rdpq_sync_tile();
    rdpq_sync_load();
    rdpq_set_mode_standard();
    rdpq_mode_zbuf(true, true);
    rdpq_mode_zoverride(true, 0, 0);
    rdpq_mode_filter(FILTER_POINT);
    rdpq_mode_alphacompare(10);
    rdpq_mode_combiner(RDPQ_COMBINER1((PRIM, 0, TEX0, 0), (0, 0, 0, TEX0)));

    // Upload texture
    rdpq_texparms_t tex_params = {
        .s.repeats = REPEAT_INFINITE,
        .t.repeats = REPEAT_INFINITE,
        .s.scale_log = 0,
        .t.scale_log = 0,
    };
    rdpq_sprite_upload(TILE0, particle_sprite, &tex_params);

    // Draw particles - use t3d state for camera/projection
    tpx_state_from_t3d();
    tpx_matrix_push(particle_matrix);
    tpx_state_set_scale(1.0f, 1.0f);

    tpx_state_set_tex_params(0, 0);

    tpx_particle_draw_tex(tpx_particles, active_count);

    tpx_matrix_pop(1);

    // Sync after TPX operations to prevent timeout errors
    rdpq_sync_pipe();
    rdpq_sync_tile();

    // // Debug display - show first active particle position
    // for (int i = 0; i < MAX_PARTICLES; i++) {
    //     if (particle_data[i].active) {
    //         draw_particle_display(particle_data[i].position);
    //         break;
    //     }
    // }
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