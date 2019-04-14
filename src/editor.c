#include "editor.h"
#include "os.h"
#include "opengl.h"
#include "draw.h"
#include "string.h"

#include <assert.h>

Font font;
String test;

u32 fps;
u32 fps_frame_count;

bool is_running;
float delta_time;
u64 last_frame_time;

void editor_init() {
	assert(gl_init());
	init_renderer();

	font = load_font("data\\fonts\\Consolas.ttf");
	test = os_load_file_into_memory("src\\draw.c");

	fps = 0;
	fps_frame_count = 0;

	is_running = true;
}

void editor_loop() {
	last_frame_time = os_get_ms_time();
	static float counter = 0.f;
	while (is_running) {
		u64 current_time = os_get_ms_time();
		delta_time = (current_time - last_frame_time) / 1000.f;
		last_frame_time = current_time;
		counter += delta_time;
		if (counter >= 1.f) {
			counter = 0.f;
			fps = fps_frame_count;
			fps_frame_count = 0;
		}

		os_poll_window_events();

		editor_draw();
	}
}

void editor_shutdown() {

}

void editor_on_window_resized(u32 old_width, u32 old_height) {
	glViewport(0, 0, os_window_width(), os_window_height());
	render_right_handed();
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
		draw_string(&test, 0.f, 0.f, 20.f, &font);
	}

	{
		// @NOTE(Colby): Text bar filling
		const float height = 20.f;
		{
			const float x0 = 0.f;
			const float y0 = window_height - height * 2.f;
			const float x1 = window_width;
			const float y1 = window_height;

			draw_rect(x0, y0, x1, y1, vec4_color(0x1a212d));
		}

		// @NOTE(Colby): Info Bar
		{
			Vector2 padding = vec2s(5.f);

			const float x0 = 0.f;
			const float y0 = window_height - height * 2.f - padding.y / 2.f;
			const float x1 = window_width;
			const float y1 = y0 + height + padding.y / 2.f;

			draw_rect(x0, y0, x1, y1, vec4_color(0x273244));

			String str = make_string("  draw.c");
			draw_string(&str, x0, y0 + padding.y / 2.f, height - 2.f, &font);
		}
	}

	fps_frame_count += 1;

	render_frame_end();
}
