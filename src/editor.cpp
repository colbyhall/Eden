#include "editor.h"

#include "os.h"
#include "buffer.h"
#include "math.h"
#include "opengl.h"
#include "draw.h"
#include "string.h"
#include "input.h"
#include "parsing.h"
#include "font.h"
#include "lua.h"

#include <assert.h>
#include <stdio.h>

Font font;

const float scroll_speed = 5.f;

Editor Editor::g_editor;

u32 frame_count = 0;


Editor& Editor::get() {
	return g_editor;
}

Buffer* Editor::create_buffer() {
	Buffer new_buffer(last_id);
	last_id += 1;
	size_t index = loaded_buffers.add(new_buffer);
	return &loaded_buffers[index];
}

Buffer* Editor::find_buffer(Buffer_ID id) {
	for (Buffer& buffer : loaded_buffers) {
		if (buffer.id == id) {
			return &buffer;
		}
	}

	return nullptr;
}

bool Editor::destroy_buffer(Buffer_ID id) {
	for (size_t i = 0; i < loaded_buffers.count; i++) {
		if (loaded_buffers[i].id == id) {
			loaded_buffers.remove(i);
			return true;
		}
	}
	
	return false;
}

void Editor::init() {
	assert(GL::init());
	init_renderer();

	Lua::get().init();

	font = Font::load_font("data\\fonts\\Consolas.ttf");
	
	Buffer* buffer = create_buffer();
	main_view.buffer = buffer;
	// buffer->init_from_size(0);
	buffer->load_from_file("src\\draw.cpp");
	buffer->title = "(YEET project) src/draw.cpp";


	is_running = true;
}

void Editor::loop() {
	last_frame_time = OS::get_ms_time();
	static float counter = 0.f;
	while (is_running) {
		u64 current_time = OS::get_ms_time();
		delta_time = (current_time - last_frame_time) / 1000.f;
		last_frame_time = current_time;
		
		counter += delta_time;
		if (counter >= 1.f) {
			counter = 0.f;
			fps = frame_count;
			frame_count = 0;
		}

		OS::poll_window_events();

		main_view.tick(delta_time);

		draw();
	}
}

void Editor::shutdown() {
	Lua::get().shutdown();
}

void Editor::on_window_resized(u32 old_width, u32 old_height) {
	glViewport(0, 0, OS::window_width(), OS::window_height());
	render_right_handed();
}

void Editor::on_mousewheel_scrolled(float delta) {
	main_view.target_scroll_y -= delta;

	if (main_view.target_scroll_y < 0.f) main_view.target_scroll_y = 0.f;

	const float font_height = FONT_SIZE;
	const float max_scroll = main_view.get_buffer_height() - font_height;
	if (main_view.target_scroll_y > max_scroll) main_view.target_scroll_y = max_scroll;
}

void Editor::on_key_pressed(u8 key) {
	Buffer* current_buffer = main_view.buffer;
	switch (key) {
	case KEY_ENTER:
		current_buffer->add_char('\n');
		break;
	case KEY_LEFT:
		current_buffer->move_cursor(-1);
		break;
	case KEY_RIGHT:
		current_buffer->move_cursor(1);
		break;
	case KEY_UP:
		current_buffer->move_cursor_line(-1);
		break;
	case KEY_DOWN:
		current_buffer->move_cursor_line(1);
		break;
	case KEY_BACKSPACE:
		current_buffer->remove_before_cursor();
		break;
	default:
		current_buffer->add_char(key);
	}
	main_view.input_pressed();
}

void Editor::on_mouse_down(Vector2 position) {
	main_view.on_mouse_down(position);
}

void Editor::on_mouse_up(Vector2 position) {
	main_view.on_mouse_up(position);
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

		draw_rect(x0, y0, x1, y1, 0x052329);
	}

	// @NOTE(Colby): Buffer drawing goes here
	{
		get_current_view()->draw();
	}

	{
		// @NOTE(Colby): Command bar filling
		float command_bar_height = FONT_SIZE + 10.f;
		{
			const float x0 = 0.f;
			const float y0 = window_height - command_bar_height;
			const float x1 = window_width;
			const float y1 = window_height;

			draw_rect(x0, y0, x1, y1, 0x052329);
			// draw_string("edit mode", x0 + 5.f, y0 + 3.f, FONT_SIZE, 0xFFFFFF);
		}
	}

	frame_count += 1;
	render_frame_end();
}