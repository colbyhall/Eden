#include "editor.h"
#include "os.h"
#include "opengl.h"
#include "draw.h"
#include "string.h"
#include "buffer.h"

#include <assert.h>

Font font;

Buffer test_buffer;

Buffer* current_buffer;

u32 fps;
u32 fps_frame_count;

bool is_running;
float delta_time;
u64 last_frame_time;

float scroll_speed;

typedef struct Buffer_View {
	Buffer* buffer;
	float desired_scroll_y;
	float current_scroll_y;
} Buffer_View;

Buffer_View test_buffer_view;

void editor_init() {
	assert(gl_init());
	init_renderer();

	font = load_font("data\\fonts\\Consolas.ttf");
	test_buffer = make_buffer();
	buffer_load_from_file(&test_buffer, "src\\draw.c");

	current_buffer = &test_buffer;

	test_buffer_view.buffer = current_buffer;
	test_buffer_view.desired_scroll_y = 0.f;
	test_buffer_view.current_scroll_y = 0.f;

	fps = 0;
	fps_frame_count = 0;
	scroll_speed = 5.f;


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



		test_buffer_view.current_scroll_y = finterpto(test_buffer_view.current_scroll_y, test_buffer_view.desired_scroll_y, delta_time, scroll_speed);

		editor_draw();
	}
}

void editor_shutdown() {

}

void editor_on_window_resized(u32 old_width, u32 old_height) {
	glViewport(0, 0, os_window_width(), os_window_height());
	render_right_handed();
}

void editor_on_mousewheel_scrolled(float delta) {
	test_buffer_view.desired_scroll_y -= delta;

	if (test_buffer_view.desired_scroll_y < 0.f) test_buffer_view.desired_scroll_y = 0.f;

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
		String out_string;
		out_string.data = current_buffer->data;
		out_string.length = current_buffer->size;
		out_string.allocated = current_buffer->size;

#if 0
		bind_shader(&font_shader);
		refresh_transformation();
		glUniform1i(font_shader.texture_loc, 0);

		glBindTexture(GL_TEXTURE_2D, font.texture_id);
		glActiveTexture(GL_TEXTURE0);

		immediate_begin();

		float space_width = 0.f;
		{
			String space_string = make_string(" ");
			space_width = get_draw_string_size(&space_string, FONT_SIZE, &font).x;
		}

		float x = 0.f;
		const float original_x = x;
		float y = -test_buffer_view.current_scroll_y;

		String line = string_eat_line(&out_string);
		while (line.length > 0) {
			String word = string_eat_until_char(&line, ' ');
			while (word.length > 0) {
				int color = 0xFFFFFF;

				if (string_equals_cstr(&word, "u32")) {
					color = 0x42f46e;
				} else if (string_equals_cstr(&word, "void")) {
					color = 0x42f46e;
				} else if (string_equals_cstr(&word, "bool")) {
					color = 0x42f46e;
				}

				Vector2 word_size = get_draw_string_size(&word, FONT_SIZE, &font);
				immediate_string(&word, x, y, FONT_SIZE, color);
				x += space_width;
				x += word_size.x;
				word = string_eat_until_char(&line, ' ');
			}
			x = original_x;
			y += FONT_SIZE;
			line = string_eat_line(&out_string);
		}

		immediate_flush();
#else
	draw_string(&out_string, 0.f, 0.f - test_buffer_view.current_scroll_y, FONT_SIZE, 0xFFFFFF);
#endif
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
			String str = make_string("ctrl-shift o");
			draw_string(&str, x0, y0 + 2.5f, FONT_SIZE, 0xFFFFFF);
		}

		// @NOTE(Colby): Info Bar
		{
			const float height = FONT_SIZE;
			Vector2 padding = vec2s(10.f);

			const float x0 = 0.f;
			const float y0 = window_height - command_bar_height - height - padding.y / 2.f;
			const float x1 = window_width;
			const float y1 = y0 + height + padding.y / 2.f;

			draw_rect(x0, y0, x1, y1, vec4_color(0x273244));

			String str = make_string(current_buffer->path);
			draw_string(&str, x0, y0, FONT_SIZE, 0xFFFFFF);
		}
	}

	fps_frame_count += 1;

	render_frame_end();
}
