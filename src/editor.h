#pragma once
#include "types.h"
#include "font.h"

#define WINDOW_TITLE "YEET"

extern Font font;
extern u32 fps;
extern float delta_time;
extern bool is_running;


void editor_init();
void editor_loop();
void editor_shutdown();

void editor_on_window_resized(u32 old_width, u32 old_height);
void editor_on_mousewheel_scrolled(float delta);
void editor_on_key_pressed(u8 key);

void editor_draw();
