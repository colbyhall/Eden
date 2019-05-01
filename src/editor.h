#pragma once
#include "types.h"
#include "array.h"
#include "buffer.h"

#define WINDOW_TITLE "YEET"

struct Font;
struct Buffer;
struct Buffer_View;
struct Vector2;

extern Font font;
extern u32 fps;
extern float dt;

extern bool is_running;

void editor_init();
void editor_loop();
void editor_shutdown();

void editor_poll_input();
void editor_tick(float dt);
void editor_draw();

void editor_on_window_resized(u32 old_width, u32 old_height);
void editor_on_mousewheel_scrolled(float delta);
void editor_on_key_pressed(u8 key);
void editor_on_mouse_down(Vector2 position);
void editor_on_mouse_up(Vector2 position);

Buffer_View* editor_get_current_view();

Buffer* editor_create_buffer();
Buffer* editor_find_buffer(Buffer_ID id);
bool editor_destroy_buffer(Buffer_ID id);


