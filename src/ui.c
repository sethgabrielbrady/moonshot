#include "ui.h"
#include "constants.h"
#include "game_state.h"
#include <rdpq.h>
#include <malloc.h>


// =============================================================================
// Tutorial Submenu State (static to avoid game_state changes)
// =============================================================================

static int tutorial_selection = 0;
static int tutorial_page = 0;

int get_tutorial_selection(void) { return tutorial_selection; }
int get_tutorial_page(void) { return tutorial_page; }
void set_tutorial_selection(int val) { tutorial_selection = val; }
void set_tutorial_page(int val) { tutorial_page = val; }

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

void draw_fps_display(float current, float avg, float min, float max) {
    int x = display_get_width() - 120;
    int y = 10;
    int line_height = 10;

    rdpq_sync_pipe();
    rdpq_text_printf(NULL, FONT_BUILTIN_DEBUG_MONO, x, y,
                     "fps: %.0f avg: %.0f", current, avg);
    struct mallinfo mem_info = mallinfo();
    int total_ram_kb = get_memory_size() / 1024;
    int heap_used_kb = mem_info.uordblks / 1024;

    y += line_height;
    rdpq_text_printf(NULL, FONT_BUILTIN_DEBUG_MONO, x, y,
                     "RAM: %dKB / %dKB", heap_used_kb, total_ram_kb);

    // y += line_height;
    // rdpq_text_printf(NULL, FONT_BUILTIN_DEBUG_MONO, x, y,
    //                  "min: %.0f max: %.0f", min, max);

}


// =============================================================================
// Pause Menu
// =============================================================================



void draw_triangle_indicator(int x, int y) {
    rdpq_set_prim_color(RGBA32(255, 165, 0, 255));  // Orange
    // Triangle pointing right (10 pixels tall)
    rdpq_fill_rectangle(x, y, x + 2, y + 10);
    rdpq_fill_rectangle(x + 2, y + 1, x + 4, y + 9);
    rdpq_fill_rectangle(x + 4, y + 2, x + 6, y + 8);
    rdpq_fill_rectangle(x + 6, y + 3, x + 8, y + 7);
    rdpq_fill_rectangle(x + 8, y + 4, x + 10, y + 6);
}

void draw_pause_menu(void) {
    int padding_x = 10;
    int padding_y = 10;
    int x1 = padding_x;
    int y1 = padding_y;
    int x2 = display_get_width() - padding_x;
    int y2 = SCREEN_HEIGHT - padding_y;

    // Draw tutorial screen if active
    if (game.show_tutorial) {
        rdpq_sync_pipe();
        rdpq_mode_combiner(RDPQ_COMBINER_FLAT);

        // Draw teal box
        rdpq_set_prim_color(RGBA32(0, 128, 128, 155));
        rdpq_fill_rectangle(x1, y1, x2, y2);

        // Draw border
        rdpq_set_prim_color(RGBA32(0, 200, 200, 255));
        rdpq_fill_rectangle(x1, y1, x2, y1 + 2);
        rdpq_fill_rectangle(x1, y2 - 2, x2, y2);
        rdpq_fill_rectangle(x1, y1, x1 + 2, y2);
        rdpq_fill_rectangle(x2 - 2, y1, x2, y2);

        int tut_x = x1 + 30;
        int tut_y = y1 + 40;
        int line_height = 14;

        // Calculate highlight position based on page/selection
        int highlight_y;
        if (tutorial_page == 0) {
            highlight_y = tut_y + (tutorial_selection * 20) - 10;
        } else {
            highlight_y = y1 + 5;  // Title position for detail pages
        }

        // Draw highlight box (shared)
        rdpq_set_prim_color(RGBA32(0, 180, 180, 255));
        rdpq_fill_rectangle(tut_x - 20, highlight_y, x2 - 15, highlight_y + 18);

        // Draw triangle indicator (shared)
        draw_triangle_indicator(tut_x - 18, highlight_y + 2);
        rdpq_sync_pipe();

        // Draw title
        const char *title;
        if (tutorial_page == 0) title = "How To Play";
        else if (tutorial_page == 1) title = "Basics";
        else if (tutorial_page == 2) title = "Ship";
        else if (tutorial_page == 3) title = "Drone";
        else if (tutorial_page == 4) title = "Resources";
        else title = "Tips";

        rdpq_text_printf(&(rdpq_textparms_t){
            .align = ALIGN_CENTER,
            .width = display_get_width(),
        }, FONT_CUSTOM, 0, y1 + 15, title);

        // Draw content based on page
        tut_x = x1 + 15;
        rdpq_sync_pipe();

        if (tutorial_page == 0) {
            // Menu options
            tut_x = x1 + 30;
            rdpq_text_printf(NULL, FONT_CUSTOM, tut_x, tut_y, "Basics");
            rdpq_text_printf(NULL, FONT_CUSTOM, tut_x, tut_y + 20, "Ship");
            rdpq_text_printf(NULL, FONT_CUSTOM, tut_x, tut_y + 40, "Drone");
            rdpq_text_printf(NULL, FONT_CUSTOM, tut_x, tut_y + 60, "Resources");
            rdpq_text_printf(NULL, FONT_CUSTOM, tut_x, tut_y + 80, "Tips");
        } else if (tutorial_page == 1) {
            // Basics content

            rdpq_text_printf(NULL, FONT_CUSTOM, tut_x, tut_y, "Mine resources to earn credits.");
            rdpq_text_printf(NULL, FONT_CUSTOM, tut_x, tut_y + line_height * 1, "Dodge and deflect asteroids.");
            rdpq_text_printf(NULL, FONT_CUSTOM, tut_x, tut_y + line_height * 2, "Command your drone to earn credits");
            rdpq_text_printf(NULL, FONT_CUSTOM, tut_x, tut_y + line_height * 4, "and repair your ship.");
            rdpq_text_printf(NULL, FONT_CUSTOM, tut_x, tut_y + line_height * 5, "If you run out of fuel or need critical");
            rdpq_text_printf(NULL, FONT_CUSTOM, tut_x, tut_y + line_height * 6, "repairs, you have 10 secs before you");
            rdpq_text_printf(NULL, FONT_CUSTOM, tut_x, tut_y + line_height * 7, "lose a credit.");
            rdpq_text_printf(NULL, FONT_CUSTOM, tut_x, tut_y + line_height * 9, "Survive as long as you can, earning");
            rdpq_text_printf(NULL, FONT_CUSTOM, tut_x, tut_y + line_height * 10, "as much as you can.");
        } else if (tutorial_page == 2) {
            // Ship content
            rdpq_text_printf(NULL, FONT_CUSTOM, tut_x, tut_y, "Move the ship with the analog stick.");
            rdpq_text_printf(NULL, FONT_CUSTOM, tut_x, tut_y + line_height * 2, "Try to conserve fuel by sliding into");
            rdpq_text_printf(NULL, FONT_CUSTOM, tut_x, tut_y + line_height * 3, "areas using your momentum.");
            rdpq_text_printf(NULL, FONT_CUSTOM, tut_x, tut_y + line_height * 5, "Deflect asteroids with the A button.");
            rdpq_text_printf(NULL, FONT_CUSTOM, tut_x, tut_y + line_height * 6, "Be careful, it also uses fuel!");
            rdpq_text_printf(NULL, FONT_CUSTOM, tut_x, tut_y + line_height * 8, "Change the camera with R/Z.");
        } else if (tutorial_page == 3) {
            // Drone content
            rdpq_text_printf(NULL, FONT_CUSTOM, tut_x, tut_y, "Command the drone with C-Buttons.");
            rdpq_text_printf(NULL, FONT_CUSTOM, tut_x, tut_y + line_height * 2, "C-Left sends it to a resource asteroid.");
            rdpq_text_printf(NULL, FONT_CUSTOM, tut_x, tut_y + line_height * 4, "C-Up sends the drone to you for repair");
            rdpq_text_printf(NULL, FONT_CUSTOM, tut_x, tut_y + line_height * 5, "but only if it has collected resources!");
            rdpq_text_printf(NULL, FONT_CUSTOM, tut_x, tut_y + line_height * 7, "C-Down sends the drone to the loading");
            rdpq_text_printf(NULL, FONT_CUSTOM, tut_x, tut_y + line_height * 8, "area to drop off its haul and earn");
            rdpq_text_printf(NULL, FONT_CUSTOM, tut_x, tut_y + line_height * 9, "your credits!");
        } else if (tutorial_page == 4) {
            // Resources content
            rdpq_text_printf(NULL, FONT_CUSTOM, tut_x, tut_y, "Mine blue resource asteroids to earn");
            rdpq_text_printf(NULL, FONT_CUSTOM, tut_x, tut_y + line_height, "credits. Move your ship to a resource");
            rdpq_text_printf(NULL, FONT_CUSTOM, tut_x, tut_y + line_height * 2, "asteroid to initiate mining.");
            rdpq_text_printf(NULL, FONT_CUSTOM, tut_x, tut_y + line_height * 4, "Be mindful to dodge and deflect");
            rdpq_text_printf(NULL, FONT_CUSTOM, tut_x, tut_y + line_height * 5, "asteroids coming at you!");
            rdpq_text_printf(NULL, FONT_CUSTOM, tut_x, tut_y + line_height * 7, "Bring what you've mined to the loading");
            rdpq_text_printf(NULL, FONT_CUSTOM, tut_x, tut_y + line_height * 8, "area for a payout, repair, and fuel.");
            rdpq_text_printf(NULL, FONT_CUSTOM, tut_x, tut_y + line_height * 10, "A full resource haul earns bonuses!");
        } else if (tutorial_page == 5) {
            // Tips content
            rdpq_text_printf(NULL, FONT_CUSTOM, tut_x, tut_y, "Conserve your fuel by taking advantage");
            rdpq_text_printf(NULL, FONT_CUSTOM, tut_x, tut_y + line_height, "of momentum.");
            rdpq_text_printf(NULL, FONT_CUSTOM, tut_x, tut_y + line_height * 3, "Watch the icons above your gauges");
            rdpq_text_printf(NULL, FONT_CUSTOM, tut_x, tut_y + line_height * 4, "for the drone status.");
            rdpq_text_printf(NULL, FONT_CUSTOM, tut_x, tut_y + line_height * 6, "Keep your drone mining - it can save");
            rdpq_text_printf(NULL, FONT_CUSTOM, tut_x, tut_y + line_height * 7, "you in a pinch!");
            rdpq_text_printf(NULL, FONT_CUSTOM, tut_x, tut_y + line_height * 9, "Look out for fuel, resources, or repair");
            rdpq_text_printf(NULL, FONT_CUSTOM, tut_x, tut_y + line_height * 10, "items floating around.");
        }

        // Back hint (shared)
        rdpq_text_printf(NULL, FONT_CUSTOM, x1 + 15, y2 - 20, "B: Back");
        return;
    }


    // Draw controls screen if active
    if (game.show_controls) {
        rdpq_sync_pipe();
        rdpq_mode_combiner(RDPQ_COMBINER_FLAT);

        // Draw teal box
        rdpq_set_prim_color(RGBA32(0, 128, 128, 155));
        rdpq_fill_rectangle(x1, y1, x2, y2);

        // Draw border
        rdpq_set_prim_color(RGBA32(0, 200, 200, 255));
        rdpq_fill_rectangle(x1, y1, x2, y1 + 2);
        rdpq_fill_rectangle(x1, y2 - 2, x2, y2);
        rdpq_fill_rectangle(x1, y1, x1 + 2, y2);
        rdpq_fill_rectangle(x2 - 2, y1, x2, y2);

        rdpq_sync_pipe();

        // Title
        rdpq_text_printf(&(rdpq_textparms_t){
            .align = ALIGN_CENTER,
            .width = display_get_width(),
        }, FONT_CUSTOM, 0, y1 + 15, "Controls");

        int ctrl_x = x1 + 15;
        int ctrl_y = y1 + 40;
        int line_height = 16;

        // Control mappings
        rdpq_text_printf(NULL, FONT_CUSTOM, ctrl_x, ctrl_y, "Stick: Move Ship");
        rdpq_text_printf(NULL, FONT_CUSTOM, ctrl_x, ctrl_y + line_height, "A: Deflect");
        rdpq_text_printf(NULL, FONT_CUSTOM, ctrl_x, ctrl_y + line_height * 2, "R/Z: Rotate Camera");
        rdpq_text_printf(NULL, FONT_CUSTOM, ctrl_x, ctrl_y + line_height * 3, "C-Up: Drone Heal");
        rdpq_text_printf(NULL, FONT_CUSTOM, ctrl_x, ctrl_y + line_height * 4, "C-Left: Drone Mine");
        rdpq_text_printf(NULL, FONT_CUSTOM, ctrl_x, ctrl_y + line_height * 5, "C-Down: Drone Return");
        rdpq_text_printf(NULL, FONT_CUSTOM, ctrl_x, ctrl_y + line_height * 6, "Start: Pause");

        // Back hint
        rdpq_text_printf(NULL, FONT_CUSTOM, ctrl_x, y2 - 20, "B: Back");
        return;
    }

    // Draw death menu (lost a life but still have lives left)
    if (game.game_over_pause && game.player_lives > 0) {
        rdpq_sync_pipe();
        rdpq_mode_combiner(RDPQ_COMBINER_FLAT);

        // Draw teal box
        rdpq_set_prim_color(RGBA32(0, 128, 128, 155));
        rdpq_fill_rectangle(x1, y1, x2, y2);

        // Draw border
        rdpq_set_prim_color(RGBA32(0, 200, 200, 255));
        rdpq_fill_rectangle(x1, y1, x2, y1 + 2);
        rdpq_fill_rectangle(x1, y2 - 2, x2, y2);
        rdpq_fill_rectangle(x1, y1, x1 + 2, y2);
        rdpq_fill_rectangle(x2 - 2, y1, x2, y2);

        rdpq_sync_pipe();

        int text_x = x1 + 15;
        int text_y = y1 + 30;
        int line_height = 16;

        // Death message
        rdpq_text_printf(NULL, FONT_CUSTOM, text_x, text_y, "Oh no buddy! You ok?");
        rdpq_text_printf(NULL, FONT_CUSTOM, text_x, text_y + line_height * 2, "You weren't out there very long.");
        rdpq_text_printf(NULL, FONT_CUSTOM, text_x, text_y + line_height * 3, "Thats ok - get out there and");
        rdpq_text_printf(NULL, FONT_CUSTOM, text_x, text_y + line_height * 4, "try again!");

        // Lives remaining
        rdpq_text_printf(NULL, FONT_CUSTOM, text_x, text_y + line_height * 6, "Lives remaining: %d", game.player_lives - 1);

        // Menu options
        int menu_y = y2 - 60;
        int option_height = 18;

        // Draw highlight box
        rdpq_set_prim_color(RGBA32(0, 180, 180, 255));
        rdpq_fill_rectangle(text_x - 5, menu_y + (game.menu_selection * option_height) - 10,
                            x2 - 15, menu_y + (game.menu_selection * option_height) + 5);

        // Draw triangle indicator
        draw_triangle_indicator(text_x - 20, menu_y + (game.menu_selection * option_height) - 8);
        rdpq_sync_pipe();

        rdpq_text_printf(NULL, FONT_CUSTOM, text_x, menu_y, "Continue");
        rdpq_text_printf(NULL, FONT_CUSTOM, text_x, menu_y + option_height, "Quit");

        return;
    }

    // Draw game over menu (no lives left)
    if (game.game_over_pause && game.player_lives <= 0) {
        rdpq_sync_pipe();
        rdpq_mode_combiner(RDPQ_COMBINER_FLAT);

        // Draw teal box
        rdpq_set_prim_color(RGBA32(0, 128, 128, 155));
        rdpq_fill_rectangle(x1, y1, x2, y2);

        // Draw border
        rdpq_set_prim_color(RGBA32(0, 200, 200, 255));
        rdpq_fill_rectangle(x1, y1, x2, y1 + 2);
        rdpq_fill_rectangle(x1, y2 - 2, x2, y2);
        rdpq_fill_rectangle(x1, y1, x1 + 2, y2);
        rdpq_fill_rectangle(x2 - 2, y1, x2, y2);

        rdpq_sync_pipe();

        int text_x = x1 + 15;
        int text_y = y1 + 30;
        int line_height = 16;

        // Game over message
        rdpq_text_printf(NULL, FONT_CUSTOM, text_x, text_y, "Well, you certainly tried.");
        rdpq_text_printf(NULL, FONT_CUSTOM, text_x, text_y + line_height * 2, "Maybe you need to take a break");
        rdpq_text_printf(NULL, FONT_CUSTOM, text_x, text_y + line_height * 3, "rather than beating up my");
        rdpq_text_printf(NULL, FONT_CUSTOM, text_x, text_y + line_height * 4, "mining ship.");

        // Menu options
        int menu_y = y2 - 60;
        int option_height = 18;

        // Draw highlight box
        rdpq_set_prim_color(RGBA32(0, 180, 180, 255));
        rdpq_fill_rectangle(text_x - 5, menu_y + (game.menu_selection * option_height) - 10,
                            x2 - 15, menu_y + (game.menu_selection * option_height) + 5);

        // Draw triangle indicator
        draw_triangle_indicator(text_x - 20, menu_y + (game.menu_selection * option_height) - 8);
        rdpq_sync_pipe();

        rdpq_text_printf(NULL, FONT_CUSTOM, text_x, menu_y, "Restart");
        rdpq_text_printf(NULL, FONT_CUSTOM, text_x, menu_y + option_height, "Quit");

        return;
    }

    if (game.game_over) {
        game.game_over_pause = true;
        game.menu_selection = 0;  // Reset to first option (Continue/Restart)
    }

    rdpq_sync_pipe();
    rdpq_mode_combiner(RDPQ_COMBINER_FLAT);

    // Draw teal box
    rdpq_set_prim_color(RGBA32(0, 128, 128, 155));
    rdpq_fill_rectangle(x1, y1, x2, y2);

    // Draw border
    rdpq_set_prim_color(RGBA32(0, 200, 200, 255));
    rdpq_fill_rectangle(x1, y1, x2, y1 + 2);
    rdpq_fill_rectangle(x1, y2 - 2, x2, y2);
    rdpq_fill_rectangle(x1, y1, x1 + 2, y2);
    rdpq_fill_rectangle(x2 - 2, y1, x2, y2);

    int menu_x = x1 + 30;
    int menu_y = y1 + 40;
    int line_height = 18;

    // Draw highlight box behind selected option
    rdpq_set_prim_color(RGBA32(0, 180, 180, 255));
    rdpq_fill_rectangle(menu_x - 20, menu_y + (game.menu_selection * line_height) - 10,
                        x2 - 15, menu_y + (game.menu_selection * line_height) + 5);

    // Draw triangle indicator
    draw_triangle_indicator(menu_x - 18, menu_y + (game.menu_selection * line_height) - 8);

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

    rdpq_text_printf(NULL, FONT_CUSTOM, menu_x, menu_y + line_height,
                     "Resolution: %s", game.hi_res_mode ? "Hi-Res" : "Lo-Res");

    const char *bgm_text;
    if (game.bgm_track == 0) bgm_text = "OFF";
    else if (game.bgm_track == 1) bgm_text = "Nebula Run";
    else if (game.bgm_track == 2) bgm_text = "Cosmic Journey";
    else if (game.bgm_track == 3) bgm_text = "Lunar Rampage";

    else bgm_text = "Random";
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
                     "Controls");
    rdpq_text_printf(NULL, FONT_CUSTOM, menu_x, menu_y + line_height * 5,
                     "Tutorial");
    rdpq_text_printf(NULL, FONT_CUSTOM, menu_x, menu_y + line_height * 6,
                     "Credits");
    // Controls hint
    rdpq_text_printf(NULL, FONT_CUSTOM, menu_x, y2 - 20,
                     "Up/Down: Select  A: Toggle");

}

void draw_game_over_screen(void) {
    // Re-use pause menu with game over state
    draw_pause_menu();
}