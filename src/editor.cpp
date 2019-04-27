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
	buffer->init_from_size(2048);


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
	// buffer_view.target_scroll_y -= delta;

	// if (buffer_view.target_scroll_y < 0.f) buffer_view.target_scroll_y = 0.f;
}

void Editor::on_key_pressed(u8 key) {
	Buffer* current_buffer = main_view.buffer;
	switch (key) {
	case KEY_ENTER:
		current_buffer->add_char('\n');
	default:
		current_buffer->add_char(key);
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

		draw_rect(x0, y0, x1, y1, 0x1a212d);
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

			draw_rect(x0, y0, x1, y1, 0x1a212d);
			draw_string("edit mode", x0 + 5.f, y0 + 3.f, FONT_SIZE, 0xFFFFFF);
		}
	}

	frame_count += 1;
	render_frame_end();
}

void Buffer_View::draw() {

	if (!buffer) return;

	const Vector2 position = get_position();
	const Vector2 size = get_size();

	font.bind();

	const float font_height = FONT_SIZE;
	float x = 0.f;
	float y = font_height - font.line_gap;

	immediate_begin();
	for (size_t i = 0; i < buffer->allocated; i++) {
		if (buffer->data[i] == '\n') {
			y += font_height;
			x = 0.f;
			continue;
		}

		if (buffer->data[i] == '\t') {
			const Font_Glyph& space_glyph = font.get_space_glyph();
			x += space_glyph.advance  * 4.f;
			continue;
		}

		if (buffer->data + i == buffer->gap) {
			i += buffer->gap_size;
		}

		const Font_Glyph& glyph = font[buffer->data[i]];

		if (!is_whitespace(buffer->data[i])) {
			Vector4 v4_color = 0xFFFFFF;

			float x0 = position.x + x + glyph.bearing_x;
			float y0 = position.y + y + glyph.bearing_y;
			float x1 = x0 + glyph.width;
			float y1 = y0 + glyph.height;

			Vector2 bottom_right = Vector2(glyph.x1 / (float)FONT_ATLAS_DIMENSION, glyph.y1 / (float)FONT_ATLAS_DIMENSION);
			Vector2 bottom_left = Vector2(glyph.x1 / (float)FONT_ATLAS_DIMENSION, glyph.y0 / (float)FONT_ATLAS_DIMENSION);
			Vector2 top_right = Vector2(glyph.x0 / (float)FONT_ATLAS_DIMENSION, glyph.y1 / (float)FONT_ATLAS_DIMENSION);
			Vector2 top_left = Vector2(glyph.x0 / (float)FONT_ATLAS_DIMENSION, glyph.y0 / (float)FONT_ATLAS_DIMENSION);

			immediate_vertex(x0, y0, v4_color, top_left);
			immediate_vertex(x0, y1, v4_color, top_right);
			immediate_vertex(x1, y0, v4_color, bottom_left);

			immediate_vertex(x0, y1, v4_color, top_right);
			immediate_vertex(x1, y1, v4_color, bottom_right);
			immediate_vertex(x1, y0, v4_color, bottom_left);
		}

		x += glyph.advance;
	}

	immediate_flush();
}

Vector2 Buffer_View::get_size() const {
	// @HACK: until we have a better system
	return Vector2((float)OS::window_width(), (float)OS::window_height() - (FONT_SIZE + 10.f));
}

Vector2 Buffer_View::get_position() const {
	return Vector2(0.f, 0.f);
}
