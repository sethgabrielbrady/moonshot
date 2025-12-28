// function to randomly change color ecvery second --- IGNORE ---
static color_t get_rainbow_color(float s) {
  // cycle through an array of 5 colors based on time
  const color_t colors[] = {
    RGBA32(255, 0, 0, COLOR_OPACITY ),    // Red
    RGBA32(0, 255, 0, COLOR_OPACITY ),    // Green
    RGBA32(0, 0, 255, COLOR_OPACITY ),    // Blue
    RGBA32(255, 255, 0, COLOR_OPACITY ),  // Yellow
    RGBA32(255, 0, 255, COLOR_OPACITY )   // Magenta
  };
  int index = ((int)s) % 5;
  return colors[index];
}

// update color opacity base on time
static color_t get_fading_color(float s) {
  // Fade between two colors based on time
  // Divide by fade period (e.g., 5.0 = 5 seconds per cycle)
  float fade_period = 5.0f;  // Change this to adjust speed
  float cycle = (s / fade_period) - (int)(s / fade_period);
  uint8_t opacity = (uint8_t)(cycle * 255);
  return RGBA32(255, 0, 0, opacity); // Red with fading opacity
}

static color_t get_distance_fading_color(Entity *entity, color_t far_color, color_t near_color) {
  float dx = camera.position.v[0] - entity->position.v[0];
  float dy = camera.position.v[1] - entity->position.v[1];
  float dz = camera.position.v[2] - entity->position.v[2];

  float distance = sqrtf(dx * dx + dy * dy + dz * dz);
  // Map distance to opacity (closer = more transparent, farther = more opaque)
  float max_distance = 300.0f; // Maximum distance where object becomes fully opaque
  float min_distance = 50.0f;  // Minimum distance where object becomes fully transparent

  // Normalize distance to 0-1 range (0 = close/near_color, 1 = far/far_color)
  float blend_factor = clampf((distance - min_distance) / (max_distance - min_distance), 0.0f, 1.0f);

  // Extract RGBA components from both colors (color_t stores as 32-bit value internally)
  uint32_t far_val = color_to_packed32(far_color);
  uint32_t near_val = color_to_packed32(near_color);

  uint8_t far_r = (far_val >> 24) & 0xFF;
  uint8_t far_g = (far_val >> 16) & 0xFF;
  uint8_t far_b = (far_val >> 8) & 0xFF;
  uint8_t far_a = far_val & 0xFF;

  uint8_t near_r = (near_val >> 24) & 0xFF;
  uint8_t near_g = (near_val >> 16) & 0xFF;
  uint8_t near_b = (near_val >> 8) & 0xFF;
  uint8_t near_a = near_val & 0xFF;

  // Interpolate between near and far colors (including opacity)
  uint8_t r = (uint8_t)(near_r + blend_factor * (far_r - near_r));
  uint8_t g = (uint8_t)(near_g + blend_factor * (far_g - near_g));
  uint8_t b = (uint8_t)(near_b + blend_factor * (far_b - near_b));
  uint8_t a = (uint8_t)(near_a + blend_factor * (far_a - near_a));

  return RGBA32(r, g, b, a);
}


static void move_ship_to_cursor(Entity *ship) {
    joypad_buttons_t pressed = joypad_get_buttons_pressed(JOYPAD_PORT_1);
    if (pressed.a && !cursor_intersecting) {
        ship->position.v[0] = cursor_position.v[0];
        ship->position.v[1] = 10.0f; //offset so entity is above ground
        ship->position.v[2] = cursor_position.v[2];
    }
}


//check intersection of cursor and entity
static void check_cursor_entity_intersection(Entity *entity) {
    color_t og_color = entity->color; // Green

    // Simple AABB check (assuming cube is axis-aligned and centered)
    float half_size = entity->scale * 100.0f; // Assuming original model size is 100 units
    if (cursor_position.v[0] >= (entity->position.v[0] - half_size) &&
        cursor_position.v[0] <= (entity->position.v[0] + half_size) &&
        cursor_position.v[2] >= (entity->position.v[2] - half_size) &&
        cursor_position.v[2] <= (entity->position.v[2] + half_size)) {
            // Intersection detected
            entity->color = RGBA32(255, 0, 0, 105); // Change color to red on intersection
            cursor_position.v[1] = entity->position.v[1] + half_size + 5.0f; // position cursor above entity
            cursor_intersecting = true;

             //if a button is held, entity position0 and entity position2 = cursor position
            joypad_buttons_t held = joypad_get_buttons_held(JOYPAD_PORT_1);
            if (held.a && cursor_intersecting) {
                cursor_grabbing = true;
                // entity->color = RGBA32(255, 0, 0, 255); // Change color to red on intersection
                update_grabbed_entity(entity);
            }
    } else {
        entity->color = og_color; // Default color
        cursor_intersecting = false;
        entity->position.v[1] = 10.0f; // reset entity height when not intersecting
        cursor_position.v[1] = 10.0f; // reset cursor height when not intersecting
        cursor_grabbing = false;
    }
}


//function moves cube to cursor position if they intersect
static void update_grabbed_entity(Entity *entity) {
    if (cursor_grabbing) {
        entity->position.v[0] = cursor_position.v[0];
        entity->position.v[1] = 10.0f; //offset so entity is above ground
        entity->position.v[2] = cursor_position.v[2];
    } else {
       //reset entity position y to default height
       entity->position.v[1] = 10.0f;
    }
}


//update entity rotation function
static void rotate_entity_y(Entity *entity, float delta_y) {
    entity->rotation_y += delta_y;
    // Keep rotation in [0, 2*PI] range
    if (entity->rotation_y >= TWO_PI) {
        entity->rotation_y -= TWO_PI;
    } else if (entity->rotation_y < 0) {
        entity->rotation_y += TWO_PI;
    }
}