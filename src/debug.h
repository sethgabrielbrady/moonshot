#ifndef DEBUG_H
#define DEBUG_H

#include "types.h"

void reset_fps_stats(void);
void update_fps_stats(float delta_time);
void render_debug_ui(T3DVec3 cursor_position, Entity entities[], Entity resources[], int resource_count, int culled_count, int cursor_resource_val, int drone_resource_val);

#endif