#include "debug.h"
#include "constants.h"
#include "utils.h"
#include "camera.h"
#include <rdpq.h>

static float fps_min = 9999.0f;
static float fps_max = 0.0f;
static float fps_current = 0.0f;
static float fps_avg = 0.0f;
static float fps_total = 0.0f;
static int fps_frame_count = 0;

void reset_fps_stats(void) {
    fps_min = 9999.0f;
    fps_max = 0.0f;
    fps_total = 0.0f;
    fps_frame_count = 0;
    fps_avg = 0.0f;
}

void update_fps_stats(float delta_time) {
    fps_current = 1.0f / delta_time;

    if (fps_current > 1.0f && fps_current < 1000.0f) {
        if (fps_current < fps_min) fps_min = fps_current;
        if (fps_current > fps_max) fps_max = fps_current;

        fps_total += fps_current;
        fps_frame_count++;
        fps_avg = fps_total / fps_frame_count;
    }
}

void render_debug_ui(T3DVec3 cursor_position, Entity *cursor_entity, bool move_drone, int resource_val, Entity entities[], int entity_count, Entity resources[], int resource_count, int culled_count, int cursor_resource_val, int drone_resource_val) {
    rdpq_sync_pipe();

    heap_stats_t stats;
    sys_get_heap_stats(&stats);

    int y = DEBUG_TEXT_Y_START;

    rdpq_text_printf(NULL, FONT_BUILTIN_DEBUG_MONO, DEBUG_TEXT_X, y,
                     "fps: %.0f avg: %.0f", fps_current, fps_avg);

    y += DEBUG_LINE_HEIGHT;
    rdpq_text_printf(NULL, FONT_BUILTIN_DEBUG_MONO, DEBUG_TEXT_X, y,
                     "min: %.0f max: %.0f", fps_min, fps_max);

    // y += DEBUG_LINE_HEIGHT;
    // rdpq_text_printf(NULL, FONT_BUILTIN_DEBUG_MONO, DEBUG_TEXT_X, y,
    //                  "ram: %dKB / %dKB", stats.used / 1024, stats.total / 1024);

    // y += DEBUG_LINE_HEIGHT;
    // rdpq_text_printf(NULL, FONT_BUILTIN_DEBUG_MONO, DEBUG_TEXT_X, y,
    //                  "cam: %.0f %.0f %.0f",
    //                  camera.position.v[0], camera.position.v[1], camera.position.v[2]);

    // y += DEBUG_LINE_HEIGHT;
    // rdpq_text_printf(NULL, FONT_BUILTIN_DEBUG_MONO, DEBUG_TEXT_X, y,
    //                  "cursor: %.0f %.0f", cursor_position.v[0], cursor_position.v[2]);

    // if (cursor_entity) {
    //     y += DEBUG_LINE_HEIGHT;
    //     rdpq_text_printf(NULL, FONT_BUILTIN_DEBUG_MONO, DEBUG_TEXT_X, y,
    //                      "dir: %s", get_compass_direction(cursor_entity->rotation_y));
    // }

    // y += DEBUG_LINE_HEIGHT;
    // rdpq_text_printf(NULL, FONT_BUILTIN_DEBUG_MONO, DEBUG_TEXT_X, y,
    //                  "drone: %s", move_drone ? "moving" : "idle");

    // y += DEBUG_LINE_HEIGHT;
    // rdpq_text_printf(NULL, FONT_BUILTIN_DEBUG_MONO, DEBUG_TEXT_X, y,
    //                  "resource_val: %d", resource_val);

    //for each resource, print its value
    // for (int i = 0; i < resource_count; i++) {
    //     y += DEBUG_LINE_HEIGHT;
    //     rdpq_text_printf(NULL, FONT_BUILTIN_DEBUG_MONO, DEBUG_TEXT_X, y,
    //                      "resource %d val: %d", i, resources[i].value);
    // }

     y += DEBUG_LINE_HEIGHT;
    rdpq_text_printf(NULL, FONT_BUILTIN_DEBUG_MONO, DEBUG_TEXT_X, y,
                     "cursor_health: %d", entities[ENTITY_CURSOR].value);

    y += DEBUG_LINE_HEIGHT;
    rdpq_text_printf(NULL, FONT_BUILTIN_DEBUG_MONO, DEBUG_TEXT_X, y,
                     "station value: %d", entities[ENTITY_STATION].value);

    y += DEBUG_LINE_HEIGHT;
    rdpq_text_printf(NULL, FONT_BUILTIN_DEBUG_MONO, DEBUG_TEXT_X, y,
                 "culled: %d", culled_count);

     y += DEBUG_LINE_HEIGHT;
    rdpq_text_printf(NULL, FONT_BUILTIN_DEBUG_MONO, DEBUG_TEXT_X, y,
                 "cursor_resource_val: %d", cursor_resource_val);

     y += DEBUG_LINE_HEIGHT;
    rdpq_text_printf(NULL, FONT_BUILTIN_DEBUG_MONO, DEBUG_TEXT_X, y,
                 "drone_resource_val: %d", drone_resource_val);

//      y += DEBUG_LINE_HEIGHT;

//     if (fabsf(cursor_velocity.v[0]) > 0.01f || fabsf(cursor_velocity.v[2]) > 0.01f) {
//         debugf("vel: %.4f, %.4f dt: %.4f\n",
//            cursor_velocity.v[0], cursor_velocity.v[2], delta_time);
// }


}