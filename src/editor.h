#pragma once
#include "types.h"
#include "font.h"
#include "buffer.h"

#define WINDOW_TITLE "YEET"

extern Font font;
extern u32 fps;
extern float delta_time;
extern bool is_running;

typedef struct Buffer_View {
	Buffer* buffer;

	float current_scroll_y;
	float target_scroll_y;

	float width;
	float height;
} Buffer_View;

typedef enum View_Container_Type {
	VCT_VERTICAL,
	VCT_HORIZONTAL
} View_Container_Type;

typedef struct View_Container {
	View_Container_Type type;
} View_Container;

void editor_init();
void editor_loop();
void editor_shutdown();
Buffer* editor_make_buffer();

void editor_on_window_resized(u32 old_width, u32 old_height);
void editor_on_mousewheel_scrolled(float delta);
void editor_on_key_pressed(u8 key);

void editor_draw();

