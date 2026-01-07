#include "debug.h"
#include "constants.h"
#include "game_state.h"
#include "utils.h"
#include "camera.h"
#include "types.h"
#include <rdpq.h>

// =============================================================================
// Debug UI Rendering
// =============================================================================

void render_debug_ui(T3DVec3 cursor_position, Entity entities[], Entity resources[], 
                     int resource_count, int culled_count, int cursor_resource_val, 
                     int drone_resource_val) {
    rdpq_sync_pipe();

    int y = DEBUG_TEXT_Y_START;

    y += DEBUG_LINE_HEIGHT;
    rdpq_text_printf(NULL, FONT_BUILTIN_DEBUG_MONO, DEBUG_TEXT_X, y,
                     "cursor: %.0f %.0f", cursor_position.v[0], cursor_position.v[2]);

    y += DEBUG_LINE_HEIGHT;
    rdpq_text_printf(NULL, FONT_BUILTIN_DEBUG_MONO, DEBUG_TEXT_X, y,
                     "cursor_health: %d", entities[ENTITY_CURSOR].value);

    y += DEBUG_LINE_HEIGHT;
    rdpq_text_printf(NULL, FONT_BUILTIN_DEBUG_MONO, DEBUG_TEXT_X, y,
                     "station value: %d", entities[ENTITY_STATION].value);

    y += DEBUG_LINE_HEIGHT;
    rdpq_text_printf(NULL, FONT_BUILTIN_DEBUG_MONO, DEBUG_TEXT_X, y,
                     "cursor_resource_val: %d", cursor_resource_val);

    y += DEBUG_LINE_HEIGHT;
    rdpq_text_printf(NULL, FONT_BUILTIN_DEBUG_MONO, DEBUG_TEXT_X, y,
                     "drone_resource_val: %d", drone_resource_val);

    y += DEBUG_LINE_HEIGHT;
    rdpq_text_printf(NULL, FONT_BUILTIN_DEBUG_MONO, DEBUG_TEXT_X, y,
                     "game_over?: %d", game.game_over ? 1 : 0);

    y += DEBUG_LINE_HEIGHT;
    rdpq_text_printf(NULL, FONT_BUILTIN_DEBUG_MONO, DEBUG_TEXT_X, y,
                     "culled: %d", culled_count);
}
