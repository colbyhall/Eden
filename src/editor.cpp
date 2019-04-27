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
	buffer->init_from_size(10);
	buffer->title = "Scratch Buffer";


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
			// draw_string("edit mode", x0 + 5.f, y0 + 3.f, FONT_SIZE, 0xFFFFFF);
		}
	}

	frame_count += 1;
	render_frame_end();
}

void Buffer_View::draw() {
	if (!buffer) return;

	const Vector2 position = get_position();
	const Vector2 size = get_size();

	// @NOTE(Colby): This is where we draw the text data
	{
		font.bind();

		const float font_height = FONT_SIZE;
		float x = 0.f;
		float y = font_height - font.line_gap;

		immediate_begin();

#if GAP_BUFFER_DEBUG
		size_t line_index = 0;

		const char* format = "%llu: LS: %llu |";

		char out_line_size[20];
		sprintf_s(out_line_size, 20, format, line_index + 1, buffer->eol_table[line_index]);

		x += immediate_string(out_line_size, position.x + x, position.y + y, 0xFFFF00).x;
#endif

		for (size_t i = 0; i < buffer->allocated; i++) {
			if (buffer->data + i == buffer->cursor) {
				immediate_flush();

				const Font_Glyph& glyph = font[buffer->data[i]];

				const float x0 = x;
				const float y0 = y - font.ascent;
				float x1 = x0 + glyph.advance;
				if (buffer->cursor == buffer->gap || is_eol(buffer->data[i])) {
					const Font_Glyph& space_glyph = font.get_space_glyph();

					x1 = x0 + space_glyph.advance;
				}
				const float y1 = y - font.descent;

				draw_rect(x0, y0, x1, y1, 0xFF00FF);

				immediate_begin();
				font.bind();
			}

			u8 c_to_draw = buffer->data[i];
			Vector4 color = 0xFFFFFF;
#if GAP_BUFFER_DEBUG
			const u8* current_position = buffer->data + i;

			if (current_position == buffer->gap) {
				c_to_draw = '[';
				color = 0x00FFFF;
			} else if (current_position == buffer->get_gap_end() - 1) {
				c_to_draw = ']';
				color = 0x00FFFF;
			} else if (current_position > buffer->gap && current_position < buffer->get_gap_end()) {
				c_to_draw = '*';
				color = 0xFFFF00;
			} else {
				if (is_eol(buffer->data[i])) {
					line_index += 1;
					x = 0.f;
					y += font_height;

					char out_line_size[20];
					sprintf_s(out_line_size, 20, format, line_index + 1, buffer->eol_table[line_index]);
					x += immediate_string(out_line_size, position.x + x, position.y + y, 0xFFFF00).x;
					continue;
				} else if (buffer->data[i] == '\t') {
					const Font_Glyph& space_glyph = font.get_space_glyph();
					x += space_glyph.advance  * 4.f;
					continue;
				} else if (buffer->data[i] == ' ') {
					x += font.get_space_glyph().advance;
					continue;
				}
			}
#else
			if (buffer->data + i >= buffer->gap && buffer->data + i < buffer->get_gap_end()) {
				continue;
			}

			if (is_eol(buffer->data[i])) {
				y += font_height;
				x = 0.f;
				continue;
			} else if (buffer->data[i] == '\t') {
				const Font_Glyph& space_glyph = font.get_space_glyph();
				x += space_glyph.advance  * 4.f;
				continue;
			} else if (buffer->data[i] == ' ') {
				x += font.get_space_glyph().advance;
				continue;
			}
#endif

			assert(!is_whitespace(c_to_draw));

			const Font_Glyph& glyph = font[c_to_draw];

			if (x + glyph.advance > size.x) {
				x = 0.f;
				y += font_height;
			}

			immediate_glyph(glyph, position.x + x, position.y + y, color);
			x += glyph.advance;
		}

		immediate_flush();
	}

	// @NOTE(Colby): We're drawing info bar here
	{
		const float bar_height = FONT_SIZE + 10.f;

		const float x0 = position.x;
		const float y0 = position.y + size.y - bar_height;
		const float x1 = x0 + size.x;
		const float y1 = y0 + bar_height;
		draw_rect(x0, y0, x1, y1, 0x273244);

		char output_string[1024];
		sprintf_s(output_string, 1024, " %s      LN: %llu     COL: %llu", buffer->title.data, buffer->current_line_number, buffer->current_column_number);
		String out_str = output_string;
		draw_string(out_str, x0, y0 + 5.f, 0xFFFFFF);
	}
}

Vector2 Buffer_View::get_size() const {
	// @HACK: until we have a better system
	return Vector2((float)OS::window_width(), (float)OS::window_height() - (FONT_SIZE + 10.f));
}

Vector2 Buffer_View::get_position() const {
	return Vector2(0.f, 0.f);
}
