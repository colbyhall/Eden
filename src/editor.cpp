#include "editor.h"

#include "os.h"
#include "buffer.h"
#include "math.h"
#include "opengl.h"
#include "draw.h"
#include "string.h"
#include "parsing.h"
#include "font.h"
#include "lua.h"

#include <assert.h>
#include <stdio.h>

Font font;
const float scroll_speed = 5.f;
u32 frame_count = 0;
u64 last_frame_time = 0;
float dt = 0.f;
u32 fps = 0;
bool is_running = false;

Array<Buffer> loaded_buffers;
u64 last_buffer_id = 0;

Buffer_View main_view;


void editor_init() {
	assert(gl_init());
	init_renderer();

	lua_init();

	font = font_load_from_file("data\\fonts\\FiraCode-Regular.ttf");
	
	Buffer* buffer = buffer_alloc();
	main_view.buffer = buffer;
	// buffer->init_from_size(0);
	buffer->load_from_file("src\\draw.cpp");
	buffer->title = "(YEET project) src/draw.cpp";


	is_running = true;
}

void editor_loop() {
	last_frame_time = os_get_ms_time();
	static float counter = 0.f;
	while (is_running) {
		u64 current_time = os_get_ms_time();
		dt = (current_time - last_frame_time) / 1000.f;
		last_frame_time = current_time;
		
		counter += dt;
		if (counter >= 1.f) {
			counter = 0.f;
			fps = frame_count;
			frame_count = 0;
		}

		editor_poll_input();

		editor_tick(dt);

		editor_draw();
	}
}

void editor_shutdown() {
	lua_shutdown();
}

void editor_poll_input() {
	os_poll_window_events();
}

void editor_tick(float dt) {
	main_view.tick(dt);
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

		draw_rect(x0, y0, x1, y1, 0x052329);
	}

	// @NOTE(Colby): Buffer drawing goes here
	{
		editor_get_current_view()->draw();
	}

	{
		// @NOTE(Colby): Command bar filling
		const float command_bar_height = window_height - Buffer_View::get_max_size().y;
		{
			const float x0 = 0.f;
			const float y0 = window_height - command_bar_height;
			const float x1 = window_width;
			const float y1 = window_height;

			draw_rect(x0, y0, x1, y1, 0x052329);
			draw_string("ctrl + alt + o", x0 + 10.f, y0, 0xFFFFFF);
		}
	}

	frame_count += 1;
	render_frame_end();
}

void editor_on_window_resized(u32 old_width, u32 old_height) {
	glViewport(0, 0, os_window_width(), os_window_height());
	render_right_handed();
}

void editor_on_mousewheel_scrolled(float delta) {
	main_view.target_scroll_y -= delta;

	if (main_view.target_scroll_y < 0.f) main_view.target_scroll_y = 0.f;

	const float font_height = FONT_SIZE;
	const float max_scroll = main_view.get_buffer_height() - font_height;
	if (main_view.target_scroll_y > max_scroll) main_view.target_scroll_y = max_scroll;
}

void editor_on_key_pressed(u8 key) {
	Buffer_View* current_view = editor_get_current_view();
	if (!current_view) {
		return;
	}
	current_view->on_key_pressed(key);
}

void editor_on_mouse_down(Vector2 position) {
	Buffer_View* current_view = editor_get_current_view();
	if (!current_view) {
		return;
	}
	current_view->on_mouse_down(position);
}

void editor_on_mouse_up(Vector2 position) {
	Buffer_View* current_view = editor_get_current_view();
	if (!current_view) {
		return;
	}
	current_view->on_mouse_up(position);
}

Buffer_View* editor_get_current_view() {
	// @TODO(Colby): This is super temp as we don't have a buffer selection thing going on
	return &main_view;
}

Buffer* editor_create_buffer() {
	Buffer new_buffer(last_buffer_id);
	last_buffer_id += 1;
	size_t index = array_add(&loaded_buffers, new_buffer);
	return &loaded_buffers[index];
}

Buffer* editor_find_buffer(Buffer_ID id) {
	for (Buffer& buffer : loaded_buffers) {
		if (buffer.id == id) {
			return &buffer;
		}
	}

	return nullptr;
}

bool editor_destroy_buffer(Buffer_ID id) {
	for (size_t i = 0; i < loaded_buffers.count; i++) {
		if (loaded_buffers[i].id == id) {
			array_remove(&loaded_buffers, i);
			return true;
		}
	}

	return false;
}