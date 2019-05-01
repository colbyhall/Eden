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
#include "input.h"

#include <assert.h>
#include <stdio.h>

Font font;
u32 frame_count = 0;
u64 last_frame_time = 0;
float dt = 0.f;
u32 fps = 0;
bool is_running = false;

Array<Buffer> loaded_buffers;
Buffer_ID last_buffer_id = 0;

Buffer_View main_view;


void editor_init() {
	assert(gl_init());
	
	draw_init();
	lua_init();

	font = font_load_from_file("data\\fonts\\FiraCode-Regular.ttf");
	
	Buffer* buffer = editor_create_buffer();
	main_view.buffer_id = buffer->id;
	buffer_load_from_file(buffer, "src\\buffer.cpp");
	// buffer_init_from_size(buffer, 1024);
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
	tick_buffer_view(&main_view, dt);
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

	// @NOTE(Colby): Command Bar and buffer drawing
	{
		const Vector2 padding = v2(10.f);
		const float font_height = FONT_SIZE;
		const float bar_height = font_height + padding.y;
		{
			const float x0 = 0.f;
			const float y0 = 0.f;
			const float x1 = x0 + window_width;
			const float y1 = y0 + window_height - bar_height;
			draw_buffer_view(main_view, x0, y0, x1, y1);
		}
		
		{
			const float x0 = 0.f;
			const float y0 = window_height - bar_height;
			const float x1 = x0 + window_width;
			const float y1 = y0 + bar_height;
			draw_rect(x0, y0, x1, y1, 0x052329);

			const float x = x0 + padding.x / 2.f;
			const float y = y0 + padding.y / 2.f;
			draw_string("esc", x, y, 0xd6b58d);
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
	const float buffer_height = buffer_view_get_buffer_height(main_view);
	const float max_scroll = buffer_height - font_height;
	if (main_view.target_scroll_y > max_scroll) main_view.target_scroll_y = max_scroll;
}

void editor_on_key_pressed(u8 key) {
	Buffer_View* current_view = editor_get_current_view();
	if (!current_view) {
		return;
	}
	Buffer* buffer = editor_find_buffer(current_view->buffer_id);
	if (!buffer) {
		return;
	}
	switch (key) {
	case KEY_ENTER:
		buffer_add_char(buffer, '\n');
		break;
	case KEY_LEFT:
		buffer_move_cursor_horizontal(buffer, -1);
		break;
	case KEY_RIGHT:
		buffer_move_cursor_horizontal(buffer, 1);
		break;
	case KEY_UP:
		buffer_move_cursor_vertical(buffer, -1);
		break;
	case KEY_DOWN:
		buffer_move_cursor_vertical(buffer, 1);
		break;
	case KEY_BACKSPACE:
		buffer_remove_before_cursor(buffer);
		break;
	default:
		buffer_add_char(buffer, key);
	}
	// current_view->on_key_pressed(key);
}

void editor_on_mouse_down(Vector2 position) {
	Buffer_View* current_view = editor_get_current_view();
	if (!current_view) {
		return;
	}

	Buffer* buffer = editor_find_buffer(current_view->buffer_id);
	if (!buffer) {
		return;
	}

	const size_t picked_index = buffer_view_pick_index(*current_view, 0.f, 0.f, position);
	buffer_set_cursor_from_index(buffer, picked_index);

	// current_view->on_mouse_down(position);
}

void editor_on_mouse_up(Vector2 position) {
	Buffer_View* current_view = editor_get_current_view();
	if (!current_view) {
		return;
	}
	// current_view->on_mouse_up(position);
}

Buffer_View* editor_get_current_view() {
	// @TODO(Colby): This is super temp as we don't have a buffer selection thing going on
	return &main_view;
}

Buffer* editor_create_buffer() {
	Buffer new_buffer = make_buffer(last_buffer_id);
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