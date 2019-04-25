#include "editor.h"
#include "os.h"
#include "opengl.h"
#include "draw.h"
#include "string.h"
#include "input.h"
#include "stretchy_buffer.h"

#include <assert.h>
#include <stdio.h>

Font font;

bool is_running = false;
float delta_time = 0.f;
u64 last_frame_time = 0;

const float scroll_speed = 5.f;

Buffer* loaded_buffers = NULL;


void editor_init() {
	assert(gl_init());
	init_renderer();

	font = load_font("data\\fonts\\Consolas.ttf");

	is_running = true;
}

void editor_loop() {
	last_frame_time = os_get_ms_time();
	while (is_running) {
		u64 current_time = os_get_ms_time();
		delta_time = (current_time - last_frame_time) / 1000.f;
		last_frame_time = current_time;

		os_poll_window_events();



		editor_draw();
	}
}

void editor_shutdown() {

}

Buffer* editor_make_buffer() {
	size_t id = buf_len(loaded_buffers);
	Buffer result = make_buffer(id);
	buf_push(loaded_buffers, result);
	return loaded_buffers + id;
}

void editor_on_window_resized(u32 old_width, u32 old_height) {
	glViewport(0, 0, os_window_width(), os_window_height());
	render_right_handed();
}

void editor_on_mousewheel_scrolled(float delta) {
	// buffer_view.target_scroll_y -= delta;

	// if (buffer_view.target_scroll_y < 0.f) buffer_view.target_scroll_y = 0.f;
}

void editor_on_key_pressed(u8 key) {
	Buffer* current_buffer = NULL;
	switch (key) {
	case KEY_ENTER:
		buffer_add_char(current_buffer, '\n');
	default:
		buffer_add_char(current_buffer, key);
	}
}

void editor_draw() {
	render_frame_begin();

	const float window_width = (float)os_window_width();
	const float window_height = (float)os_window_height();
	
	// @NOTE(Colby): Background
	{
		const float x0 = 0.f;
		const float y0 = 0.f;
		const float x1 = window_width;
		const float y1 = window_height;

		draw_rect(x0, y0, x1, y1, vec4_color(0x1a212d));
	}

	// @NOTE(Colby): Buffer drawing goes here
	{

	}

	{
		// @NOTE(Colby): Command bar filling
		float command_bar_height = FONT_SIZE + 10.f;
		{
			const float x0 = 0.f;
			const float y0 = window_height - command_bar_height;
			const float x1 = window_width;
			const float y1 = window_height;

			draw_rect(x0, y0, x1, y1, vec4_color(0x1a212d));
			String str = make_string("edit mode");
			draw_string(&str, x0 + 5.f, y0 + 3.f, FONT_SIZE, 0xFFFFFF);
		}
	}

	render_frame_end();
}