#ifndef INPUT_H
#define INPUT_H

#include <libdragon.h>
#include "types.h"

// =============================================================================
// Input State Structure
// =============================================================================

typedef struct {
    float stick_x;
    float stick_y;
    float stick_magnitude;
    float stick_magnitude_sq;
    joypad_buttons_t pressed;
    joypad_buttons_t held;
    joypad_buttons_t released;
} InputState;

// =============================================================================
// Global Input State
// =============================================================================

extern InputState input;

// =============================================================================
// Functions
// =============================================================================

// Call once per frame to update input state
void update_input(void);

// Process game input (movement, actions) - call when not paused
void process_game_input(float delta_time);

// Process menu input - call when paused
void process_menu_input(void);

// Process debug/system input - call always
void process_system_input(T3DViewport *viewport);

// Update cursor movement based on input
void update_cursor_movement(float delta_time, Entity *cursor_entity);

#endif // INPUT_H
