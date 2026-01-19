#include "ui.h"
#include "constants.h"
#include "game_state.h"
#include <rdpq.h>

// =============================================================================
// FPS Stats
// =============================================================================


FPSStats fps_stats = {
    .current = 0.0f,
    .avg = 0.0f,
    .min = 0.0f,
    .max = 0.0f,
    .frame_times = {0},      // Add this
    .frame_index = 0,        // Add this
    .sample_count = 0,       // Add this
    .frame_count = 0
};

void reset_fps_stats(void) {
    fps_stats.min = 9999.0f;
    fps_stats.max = 0.0f;
    fps_stats.frame_count = 0;
    fps_stats.avg = 0.0f;
}

void update_fps_stats(float delta_time) {
    // Use delta_time directly - it's the actual time between frames
    float current_fps = (delta_time > 0.0f) ? (1.0f / delta_time) : 0.0f;

    // Clamp to reasonable values to avoid spikes
    if (current_fps > 200.0f) current_fps = 200.0f;
    if (current_fps < 1.0f) current_fps = 1.0f;

    fps_stats.current = current_fps;
    fps_stats.frame_times[fps_stats.frame_index] = current_fps;
    fps_stats.frame_index = (fps_stats.frame_index + 1) % FPS_SAMPLE_WINDOW;

    if (fps_stats.sample_count < FPS_SAMPLE_WINDOW) {
        fps_stats.sample_count++;
    }

    // Calculate average over last 6 seconds
    float sum = 0.0f;
    fps_stats.min = 9999.0f;
    fps_stats.max = 0.0f;

    for (int i = 0; i < fps_stats.sample_count; i++) {
        float fps = fps_stats.frame_times[i];
        sum += fps;
        if (fps < fps_stats.min) fps_stats.min = fps;
        if (fps > fps_stats.max) fps_stats.max = fps;
    }

    fps_stats.avg = sum / fps_stats.sample_count;
}
// =============================================================================
// FPS Display
// =============================================================================

void draw_fps_display(float current, float avg, float min, float max, int particle_count) {
    int x = display_get_width() - 120;
    int y = 10;
    int line_height = 10;

    rdpq_sync_pipe();

    rdpq_text_printf(NULL, FONT_BUILTIN_DEBUG_MONO, x, y,
                     "fps: %.0f avg: %.0f", current, avg);
    y += line_height;
    rdpq_text_printf(NULL, FONT_BUILTIN_DEBUG_MONO, x, y,
                     "min: %.0f max: %.0f", min, max);
    y += line_height;
    rdpq_text_printf(NULL, FONT_BUILTIN_DEBUG_MONO, x, y,
                     "Particles: %d", particle_count);
}


// =============================================================================
// Pause Menu
// =============================================================================


void draw_triangle_indicator(int x, int y) {
    rdpq_set_prim_color(RGBA32(255, 165, 0, 255));  // Orange

    // Large triangle pointing right (14 pixels tall)
    rdpq_fill_rectangle(x, y + 6, x + 2, y + 8);
    rdpq_fill_rectangle(x + 2, y + 5, x + 4, y + 9);
    rdpq_fill_rectangle(x + 4, y + 4, x + 6, y + 10);
    rdpq_fill_rectangle(x + 6, y + 3, x + 8, y + 11);
    rdpq_fill_rectangle(x + 8, y + 2, x + 10, y + 12);
    rdpq_fill_rectangle(x + 10, y + 1, x + 12, y + 13);
    rdpq_fill_rectangle(x + 12, y, x + 14, y + 14);
}

void draw_pause_menu(void) {

    if (game.game_over) {
        // game.game_over = false;
        game.game_over_pause = true;
    }
    int padding_x = display_get_width() * 0.2f;
    int padding_y = SCREEN_HEIGHT * 0.15f;

    int x1 = padding_x;
    int y1 = padding_y;
    int x2 = display_get_width() - padding_x;
    int y2 = SCREEN_HEIGHT - padding_y;

    rdpq_sync_pipe();
    rdpq_mode_combiner(RDPQ_COMBINER_FLAT);

    // Draw teal box
    rdpq_set_prim_color(RGBA32(0, 128, 128, 255));
    rdpq_fill_rectangle(x1, y1, x2, y2);

    // Draw border
    rdpq_set_prim_color(RGBA32(0, 200, 200, 255));
    rdpq_fill_rectangle(x1, y1, x2, y1 + 2);
    rdpq_fill_rectangle(x1, y2 - 2, x2, y2);
    rdpq_fill_rectangle(x1, y1, x1 + 2, y2);
    rdpq_fill_rectangle(x2 - 2, y1, x2, y2);

    int menu_x = x1 + 15;
    int menu_y = y1 + 40;
    int line_height = 18;

    // Draw highlight box behind selected option
    rdpq_set_prim_color(RGBA32(0, 180, 180, 255));
    rdpq_fill_rectangle(menu_x - 5, menu_y + (game.menu_selection * line_height) - 10,
                        x2 - 15, menu_y + (game.menu_selection * line_height) + 5);

    rdpq_sync_pipe();

    // Title - show lives remaining if game over
    if (game.game_over_pause) {
        rdpq_text_printf(&(rdpq_textparms_t){
            .align = ALIGN_CENTER,
            .width = display_get_width(),
        }, FONT_CUSTOM, 0, y1 + 15, "Lives: %d", game.player_lives);
    } else {
        rdpq_text_printf(&(rdpq_textparms_t){
            .align = ALIGN_CENTER,
            .width = display_get_width(),
        }, FONT_CUSTOM, 0, y1 + 15, "AsteRisk");
    }

    // Menu options
    const char *first_option;
    if (game.game_over_pause) {
        first_option = (game.player_lives > 1) ? "Continue" : "Restart";
    } else {
        first_option = "Resume";
    }
    rdpq_text_printf(NULL, FONT_CUSTOM, menu_x, menu_y, "%s", first_option);

    // rdpq_text_printf(NULL, FONT_CUSTOM, menu_x, menu_y + line_height,
    //                  "Camera: %s", game.fps_mode ? "FPS" : "ISO");

    rdpq_text_printf(NULL, FONT_CUSTOM, menu_x, menu_y + line_height ,
                     "Resolution: %s", game.hi_res_mode ? "640x240" : "320x240");

    const char *bgm_text;
    if (game.bgm_track == 0) bgm_text = "OFF";
    else if (game.bgm_track == 1) bgm_text = "Nebula Run";
    else if (game.bgm_track == 2) bgm_text = "Orbit Oddyssey";
    else bgm_text = "Lunar Rampage";
    rdpq_text_printf(NULL, FONT_CUSTOM, menu_x, menu_y + line_height * 2,
                     "Music: %s", bgm_text);

    const char *fps_text;
    if (game.is_pal_system) {
        if (game.fps_limit == 0) fps_text = "25";
        else if (game.fps_limit == 1) fps_text = "50";
        else fps_text = "Uncapped";
    } else {
        if (game.fps_limit == 0) fps_text = "30";
        else if (game.fps_limit == 1) fps_text = "60";
        else fps_text = "Uncapped";
    }
    rdpq_text_printf(NULL, FONT_CUSTOM, menu_x, menu_y + line_height * 3,
                     "FPS Limit: %s", fps_text);
    rdpq_text_printf(NULL, FONT_CUSTOM, menu_x, menu_y + line_height * 4,
                     "Background: %s", game.render_background_enabled ? "ON" : "OFF");

    // Controls hint
    rdpq_text_printf(NULL, FONT_CUSTOM, menu_x, y2 - 20,
                     "Up/Down: Select  A: Toggle");
}

void draw_game_over_screen(void) {
    // Re-use pause menu with game over state
    draw_pause_menu();
}