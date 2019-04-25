#include "editor.h"
#include "os.h"
#include "opengl.h"
#include "draw.h"
#include "string.h"
#include "input.h"

#include <assert.h>
#include <stdio.h>

Font font;

const float scroll_speed = 5.f;

Buffer* loaded_buffers = NULL;

Editor Editor::g_editor;


Editor& Editor::get() {
	return g_editor;
}

void Editor::init() {
	assert(gl_init());
	init_renderer();

	font = load_font("data\\fonts\\Consolas.ttf");

	is_running = true;
}

void Editor::loop() {
	last_frame_time = OS::get_ms_time();
	while (is_running) {
		u64 current_time = OS::get_ms_time();
		delta_time = (current_time - last_frame_time) / 1000.f;
		last_frame_time = current_time;

		OS::poll_window_events();

		draw();
	}
}

void Editor::shutdown() {

}

void Editor::on_window_resized(u32 old_width, u32 old_height) {
	glViewport(0, 0, OS::window_width(), OS::window_height());
	render_right_handed();
}

void Editor::on_mousewheel_scrolled(float delta) {
	// buffer_view.target_scroll_y -= delta;

	// if (buffer_view.target_scroll_y < 0.f) buffer_view.target_scroll_y = 0.f;
}

void Editor::on_key_pressed(u8 key) {
	Buffer* current_buffer = NULL;
	switch (key) {
	case KEY_ENTER:
		buffer_add_char(current_buffer, '\n');
	default:
		buffer_add_char(current_buffer, key);
	}
}

void Editor::draw() {
	render_frame_begin();

	const float window_width = (float)OS::window_width();
	const float window_height = (float)OS::window_height();
	
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
			draw_string("edit mode", x0 + 5.f, y0 + 3.f, FONT_SIZE, 0xFFFFFF);
		}
	}

	render_frame_end();
}