#ifndef DEBUG_H
#define DEBUG_H

#include "types.h"

// =============================================================================
// Debug UI Rendering
// =============================================================================

void render_debug_ui(T3DVec3 cursor_position, Entity entities[], Entity resources[], 
                     int resource_count, int culled_count, int cursor_resource_val, 
                     int drone_resource_val);

#endif // DEBUG_H
