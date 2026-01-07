#include "particles.h"
#include <t3d/t3d.h>
#include <t3d/tpx.h>
#include <stdlib.h>
#include <math.h>
#include "types.h"
#include "constants.h"
#include "camera.h"

#define MAX_PARTICLES 512
#define MAX_AMBIENT_PARTICLES 256

int debug_particle_count = 0;

typedef struct {
    T3DVec3 position;
    T3DVec3 velocity;
    float speed;
    float size;
    bool active;
} AmbientParticle;

static AmbientParticle ambient_particles[MAX_AMBIENT_PARTICLES];


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
    for (int i = 0; i < 16; i++) {  // 20 is even
        T3DVec3 velocity = {{
            (rand() % 250 - 100) * 1.0f,
            (rand() % 80 + 20) * 1.0f,
            (rand() % 250 - 100) * 1.0f
        }};
        // color_t color = RGBA32(255, 150 + (rand() % 105), 50, 255);
        // float size = 0.010f + (rand() % 20) * 0.01f;


        // if fps_mode is enabled, make particles smaller
        spawn_particle(position, velocity, COLOR_SPARKS, 0.04f , 0.4f);
    }
}

void spawn_mining_sparks(T3DVec3 position) {
    for (int i = 0; i < 4; i++) {  // Changed from 5 to 6 (even)
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
//         color_t color = RGBA32(255, 248, 4, 125);
//         spawn_particle(position, velocity, color, 0.02f, 0.3f);
//     }
// }
// void update_particles(float delta_time) {
//     for (int i = 0; i < MAX_PARTICLES; i++) {
//         if (!particle_data[i].active) continue;

//         ParticleData *p = &particle_data[i];

//         // Update lifetime
//         p->lifetime -= delta_time;
//         if (p->lifetime <= 0) {
//             p->active = false;
//             continue;
//         }

//         // Apply gravity
//         p->velocity.v[1] -= 150.0f * delta_time;

//         // Apply drag
//         p->velocity.v[0] *= 0.98f;
//         p->velocity.v[1] *= 0.99f;
//         p->velocity.v[2] *= 0.98f;

//         // Update position
//         p->position.v[0] += p->velocity.v[0] * delta_time;
//         p->position.v[1] += p->velocity.v[1] * delta_time;
//         p->position.v[2] += p->velocity.v[2] * delta_time;
//     }
// }
// In update_particles - early exit if no particles
void update_particles(float delta_time) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        ParticleData *p = &particle_data[i];
        if (!p->active) continue;

        // Update lifetime first
        p->lifetime -= delta_time;
        if (p->lifetime <= 0) {
            p->active = false;
            continue;
        }

        // Apply gravity and drag together
        p->velocity.v[1] -= 150.0f * delta_time;

        float drag = 0.98f;
        p->velocity.v[0] *= drag;
        p->velocity.v[1] *= 0.99f;
        p->velocity.v[2] *= drag;

        // Update position
        p->position.v[0] += p->velocity.v[0] * delta_time;
        p->position.v[1] += p->velocity.v[1] * delta_time;
        p->position.v[2] += p->velocity.v[2] * delta_time;
    }
}


// void draw_particles(T3DViewport *viewport) {
//     if (!particle_sprite || !tpx_particles) return;

//     // Build TPX particle array from active particles
//     int active_count = 0;
//     TPXParticle *tpx = tpx_particles;

//     for (int i = 0; i < MAX_PARTICLES; i++) {
//         if (!particle_data[i].active) continue;

//         ParticleData *p = &particle_data[i];

//         // Frustum culling - skip particles far off-screen
//         float dx = p->position.v[0] - camera.position.v[0];
//         float dz = p->position.v[2] - camera.position.v[2];
//         float dist_sq = dx * dx + dz * dz;
//         if (dist_sq > 1000000.0f) continue;  // Skip particles >1000 units away

//         float life_ratio = p->lifetime / p->max_lifetime;

//         // Fade alpha based on lifetime
//         uint8_t alpha = (uint8_t)(life_ratio * 255);

//         // Scale size based on lifetime
//         float size = p->size * (0.5f + life_ratio * 0.5f);
//         int8_t size_fp = (int8_t)(size * 64);

//         // Position - TPX uses int8_t for small offsets relative to the matrix
//         // Scale world positions down to fit in int8_t range (-128 to 127)
//         int8_t px = (int8_t)(p->position.v[0] * 0.1f);
//         int8_t py = (int8_t)(p->position.v[1] * 0.1f);
//         int8_t pz = (int8_t)(p->position.v[2] * 0.1f);

//         // Fill both A and B at once
//         tpx->posA[0] = tpx->posB[0] = px;
//         tpx->posA[1] = tpx->posB[1] = py;
//         tpx->posA[2] = tpx->posB[2] = pz;
//         tpx->sizeA = tpx->sizeB = size_fp;

//         // Fill color arrays efficiently
//         uint8_t r = p->color.r, g = p->color.g, b = p->color.b;
//         tpx->colorA[0] = tpx->colorB[0] = r;
//         tpx->colorA[1] = tpx->colorB[1] = g;
//         tpx->colorA[2] = tpx->colorB[2] = b;
//         tpx->colorA[3] = tpx->colorB[3] = alpha;

//         tpx++;
//         active_count++;
//     }

//     // Add ambient particles
//     for (int i = 0; i < MAX_AMBIENT_PARTICLES; i++) {
//         AmbientParticle *ap = &ambient_particles[i];
//         if (!ap->active) continue;

//         // Frustum culling - skip particles far from camera
//         float dx = ap->position.v[0] - camera.position.v[0];
//         float dz = ap->position.v[2] - camera.position.v[2];
//         float dist_sq = dx * dx + dz * dz;
//         if (dist_sq > 1500000.0f) continue;  // Skip particles >1225 units away

//         // Use particle's random size
//         int8_t size_fp = (int8_t)(ap->size * 64);

//         // Position - scale down to fit in int8_t range
//         int8_t px = (int8_t)(ap->position.v[0] * 0.1f);
//         int8_t py = (int8_t)(ap->position.v[1] * 0.1f);
//         int8_t pz = (int8_t)(ap->position.v[2] * 0.1f);

//         // Fill both A and B
//         tpx->posA[0] = tpx->posB[0] = px;
//         tpx->posA[1] = tpx->posB[1] = py;
//         tpx->posA[2] = tpx->posB[2] = pz;
//         tpx->sizeA = tpx->sizeB = size_fp;

//         // Magenta color
//         tpx->colorA[0] = tpx->colorB[0] = 101;
//         tpx->colorA[1] = tpx->colorB[1] = 67;
//         tpx->colorA[2] = tpx->colorB[2] = 33;
//         tpx->colorA[3] = tpx->colorB[3] = 255;

//         tpx++;
//         active_count++;
//     }

//     debug_particle_count = active_count;

//     // TPX requires even number of particles
//     if (active_count & 1) {
//         active_count--;
//     }

//     if (active_count < 2) return;

//     // Matrix with 10x scale to compensate for 0.1x scale of positions above
//     t3d_mat4fp_from_srt_euler(particle_matrix,
//         (float[]){10.0f, 10.0f, 10.0f},
//         (float[]){0, 0, 0},
//         (float[]){0, 0, 0}
//     );

//     // Set up rendering state
//     rdpq_sync_pipe();
//     rdpq_sync_tile();
//     rdpq_sync_load();
//     rdpq_set_mode_standard();
//     rdpq_mode_zbuf(true, true);
//     rdpq_mode_zoverride(true, 0, 0);
//     rdpq_mode_filter(FILTER_POINT);
//     rdpq_mode_alphacompare(10);
//     rdpq_mode_combiner(RDPQ_COMBINER1((PRIM, 0, TEX0, 0), (0, 0, 0, TEX0)));

//     // Upload texture
//     rdpq_texparms_t tex_params = {
//         .s.repeats = REPEAT_INFINITE,
//         .t.repeats = REPEAT_INFINITE,
//         .s.scale_log = 0,
//         .t.scale_log = 0,
//     };
//     rdpq_sprite_upload(TILE0, particle_sprite, &tex_params);

//     // Draw particles - use t3d state for camera/projection
//     tpx_state_from_t3d();
//     tpx_matrix_push(particle_matrix);
//     tpx_state_set_scale(1.0f, 1.0f);

//     tpx_state_set_tex_params(0, 0);

//     tpx_particle_draw_tex(tpx_particles, active_count);

//     tpx_matrix_pop(1);

//     // Sync after TPX operations to prevent timeout errors
//     rdpq_sync_pipe();
//     rdpq_sync_tile();

//     // // Debug display - show first active particle position
//     // for (int i = 0; i < MAX_PARTICLES; i++) {
//     //     if (particle_data[i].active) {
//     //         draw_particle_display(particle_data[i].position);
//     //         break;
//     //     }
//     // }
// }


void draw_particles(T3DViewport *viewport) {
    if (!particle_sprite || !tpx_particles) return;

    int active_count = 0;
    TPXParticle *tpx = tpx_particles;

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

        // Position RELATIVE to camera, then scale to fit int8_t
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

    // Same for ambient particles
    for (int i = 0; i < MAX_AMBIENT_PARTICLES; i++) {
        AmbientParticle *ap = &ambient_particles[i];
        if (!ap->active) continue;

        float dx = ap->position.v[0] - camera.position.v[0];
        float dz = ap->position.v[2] - camera.position.v[2];
        float dist_sq = dx * dx + dz * dz;
        if (dist_sq > 1500000.0f) continue;

        int8_t size_fp = (int8_t)(ap->size * 64);

        // Position relative to camera
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

        tpx->colorA[0] = tpx->colorB[0] = 101;
        tpx->colorA[1] = tpx->colorB[1] = 67;
        tpx->colorA[2] = tpx->colorB[2] = 33;
        tpx->colorA[3] = tpx->colorB[3] = 255;

        tpx++;
        active_count++;
    }

    debug_particle_count = active_count;

    if (active_count & 1) {
        active_count--;
    }

    if (active_count < 2) return;

    // Matrix positioned at camera with 10x scale
    t3d_mat4fp_from_srt_euler(particle_matrix,
        (float[]){10.0f, 10.0f, 10.0f},
        (float[]){0, 0, 0},
        (float[]){camera.position.v[0], camera.position.v[1], camera.position.v[2]}
    );

    // Rest unchanged...
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

void init_ambient_particles(void) {
    for (int i = 0; i < MAX_AMBIENT_PARTICLES; i++) {
        AmbientParticle *p = &ambient_particles[i];
        p->active = true;

        // Random position across the play area
        p->position.v[0] = (rand() % ((int)PLAY_AREA_SIZE * 2)) - PLAY_AREA_SIZE;
        p->position.v[1] = 24.0f + (rand() % 77);  // Random height between 24 and 100
        p->position.v[2] = (rand() % ((int)PLAY_AREA_SIZE * 2)) - PLAY_AREA_SIZE;

        // Random direction
        float angle = (rand() % 360) * (3.14159f / 180.0f);
        p->velocity.v[0] = cosf(angle);
        p->velocity.v[1] = 0;
        p->velocity.v[2] = sinf(angle);

        // Random speed between 10-30 units
        p->speed = 10.0f + (rand() % 20);

        // Random size from 0.01f to 0.04f (half to double the base 0.02f)
        p->size = (10 + (rand() % 31)) * 0.001f;
    }
}

void update_ambient_particles(float delta_time) {
    for (int i = 0; i < MAX_AMBIENT_PARTICLES; i++) {
        AmbientParticle *p = &ambient_particles[i];
        if (!p->active) continue;

        // Move particle
        p->position.v[0] += p->velocity.v[0] * p->speed * delta_time;
        p->position.v[2] += p->velocity.v[2] * p->speed * delta_time;

        // Check if out of bounds and respawn at random edge
        bool out_of_bounds = false;

        if (p->position.v[0] > PLAY_AREA_SIZE || p->position.v[0] < -PLAY_AREA_SIZE ||
            p->position.v[2] > PLAY_AREA_SIZE || p->position.v[2] < -PLAY_AREA_SIZE) {
            out_of_bounds = true;
        }

        if (out_of_bounds) {
            // Respawn at a random edge
            int edge = rand() % 4;
            switch (edge) {
                case 0: // Top edge
                    p->position.v[0] = (rand() % ((int)PLAY_AREA_SIZE * 2)) - PLAY_AREA_SIZE;
                    p->position.v[1] = 5.0f + (rand() % 77);  // Random height
                    p->position.v[2] = -PLAY_AREA_SIZE;
                    break;
                case 1: // Bottom edge
                    p->position.v[0] = (rand() % ((int)PLAY_AREA_SIZE * 2)) - PLAY_AREA_SIZE;
                    p->position.v[1] = 5.0f + (rand() % 77);  // Random height
                    p->position.v[2] = PLAY_AREA_SIZE;
                    break;
                case 2: // Left edge
                    p->position.v[0] = -PLAY_AREA_SIZE;
                    p->position.v[1] = 5.0f + (rand() % 77);  // Random height
                    p->position.v[2] = (rand() % ((int)PLAY_AREA_SIZE * 2)) - PLAY_AREA_SIZE;
                    break;
                case 3: // Right edge
                    p->position.v[0] = PLAY_AREA_SIZE;
                    p->position.v[1] = 5.0f + (rand() % 77);  // Random height
                    p->position.v[2] = (rand() % ((int)PLAY_AREA_SIZE * 2)) - PLAY_AREA_SIZE;
                    break;
            }


            // New random direction
            float angle = (rand() % 360) * (3.14159f / 180.0f);
            p->velocity.v[0] = cosf(angle);
            p->velocity.v[2] = sinf(angle);
            p->speed = 10.0f + (rand() % 20);
            p->size = (10 + (rand() % 31)) * 0.001f;
        }
    }
}

// Large explosion that clears other particles
// void spawn_station_explosion(T3DVec3 position) {
//     // Clear all existing particles first
//     clear_all_particles();
//     trigger_screen_shake(5.0f, 0.5f);  // Intensity 5, duration 0.5

//     // Spawn large explosion - more particles, bigger, longer lasting
//     for (int i = 0; i < 64; i++) {
//         // Spherical burst pattern
//         float angle_h = (rand() % 360) * (3.14159f / 180.0f);
//         float angle_v = (rand() % 180 - 90) * (3.14159f / 180.0f);
//         float speed = 100.0f + (rand() % 150);

//         T3DVec3 velocity = {{
//             cosf(angle_h) * cosf(angle_v) * speed,
//             sinf(angle_v) * speed + 50.0f,  // Bias upward
//             sinf(angle_h) * cosf(angle_v) * speed
//         }};

//         // Mix of orange, red, yellow colors
//         color_t color;
//         int color_type = rand() % 3;
//         switch (color_type) {
//             case 0: color = RGBA32(255, 200, 50, 255); break;   // Yellow
//             case 1: color = RGBA32(255, 100, 0, 255); break;    // Orange
//             case 2: color = RGBA32(255, 50, 50, 255); break;    // Red
//         }

//         float size = 0.06f + (rand() % 40) * 0.001f;  // Bigger particles
//         float lifetime = 1.0f + (rand() % 100) * 0.01f;  // 1.0 - 2.0 seconds

//         spawn_particle(position, velocity, color, size, lifetime);
//     }

//     // Add secondary debris particles
//     for (int i = 0; i < 32; i++) {
//         T3DVec3 velocity = {{
//             (rand() % 300 - 150) * 1.0f,
//             (rand() % 200) * 1.0f,
//             (rand() % 300 - 150) * 1.0f
//         }};

//         color_t color = RGBA32(150, 150, 150, 255);  // Gray debris
//         float size = 0.02f + (rand() % 20) * 0.001f;
//         float lifetime = 0.8f + (rand() % 50) * 0.01f;

//         spawn_particle(position, velocity, color, size, lifetime);
//     }
// }

void spawn_station_explosion(T3DVec3 position) {
    clear_all_particles();
    trigger_screen_shake(8.0f, 1.0f);  // Longer shake
    trigger_camera_zoom(position, 3.0f);  // Zoom for 3 seconds

    // Primary explosion - longer lifetime
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
        float lifetime = 2.0f + (rand() % 150) * 0.01f;  // 2.0 - 3.5 seconds (longer)

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
        float lifetime = 1.5f + (rand() % 100) * 0.01f;  // 1.5 - 2.5 seconds

        spawn_particle(position, velocity, color, size, lifetime);
    }

    // Debris particles - slowest, longest lasting
    for (int i = 0; i < 30; i++) {
        T3DVec3 velocity = {{
            (rand() % 400 - 200) * 0.5f,  // Slower
            (rand() % 250) * 0.5f,
            (rand() % 400 - 200) * 0.5f
        }};

        color_t color = RGBA32(180, 180, 180, 255);
        float size = 0.04f + (rand() % 30) * 0.001f;
        float lifetime = 2.5f + (rand() % 150) * 0.01f;  // 2.5 - 4.0 seconds

        spawn_particle(position, velocity, color, size, lifetime);
    }
}