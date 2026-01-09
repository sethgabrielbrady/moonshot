#ifndef UI_H
#define UI_H

#include "types.h"

// =============================================================================
// Health and Resource Bars
// =============================================================================

void draw_health_bars(Entity *station, Entity *cursor);
void draw_resource_bars(void);
// void draw_info_bars(Entity *station, Entity *cursor);

// =============================================================================
// Status Indicators
// =============================================================================

void draw_drone_status_icon(int x, int y);
void draw_triangle_indicator(int x, int y);
void draw_circle_indicator(int x, int y);
void draw_station_indicator(int x, int y);

// =============================================================================
// FPS Display
// =============================================================================

void draw_fps_display(float current, float avg, float min, float max, int particle_count);

// =============================================================================
// Menus
// =============================================================================

void draw_pause_menu(void);
void draw_game_over_screen(void);

// =============================================================================
// FPS Stats Tracking
// =============================================================================

#define FPS_SAMPLE_WINDOW 180  // 6 seconds at 30fps

typedef struct {
    float current;
    float avg;
    float min;
    float max;
    float frame_times[FPS_SAMPLE_WINDOW];  // Add this
    int frame_index;                        // Add this
    int sample_count;                       // Add this
    uint32_t frame_count;
} FPSStats;

extern FPSStats fps_stats;

void reset_fps_stats(void);
void update_fps_stats(float delta_time);

#endif // UI_H
