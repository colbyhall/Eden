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

struct Editor {
	static Editor& get();

	bool is_running = false;
	float delta_time = 0.f;
	u64 last_frame_time = 0;
	u32 fps;

	void init();
	void loop();
	void shutdown();

	void on_window_resized(u32 old_width, u32 old_height);
	void on_mousewheel_scrolled(float delta);
	void on_key_pressed(u8 key);
	
	void draw();
private:
	static Editor g_editor;
};



