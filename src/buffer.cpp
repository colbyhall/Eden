#include "buffer.h"
#include "parsing.h"
#include "string.h"
#include "os.h"
#include "memory.h"
#include "draw.h"
#include "font.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define DEFAULT_GAP_SIZE 1024

Buffer::Buffer(Buffer_ID id) 
	: id(id), modified(false), allocated(0), gap(nullptr), gap_size(0), current_line_number(0), desired_column_number(0), current_column_number(0), cursor(0) {}

void Buffer::move_gap_to_cursor() {
	if (cursor == gap) return;

	assert(cursor < gap || cursor > get_gap_end() - 1);

	if (cursor < gap) {
		const size_t amount_to_move = gap - cursor;

		memcpy(get_gap_end() - amount_to_move, cursor, amount_to_move);

		gap = cursor;
	} else {
		const size_t amount_to_move = cursor - get_gap_end();

		memcpy(gap, get_gap_end(), amount_to_move);

		gap += amount_to_move;
		cursor = gap;
	}
}

void Buffer::refresh_cursor_info(bool update_desired) {
	current_line_number = 0;
	current_column_number = 0;
	for (size_t i = 0; i < get_size(); i++) {
		const u8* pos = get_position(i);
		if (pos == cursor || (cursor == gap && get_gap_end() == pos)) {
			break;
		}

		current_column_number += 1;

		if (is_eol(*pos)) {
			current_line_number += 1;
			current_column_number = 0;
		}
	}

	if (update_desired) {
		desired_column_number = current_column_number;
	}
}

void Buffer::refresh_eol_table(size_t reserve_size) {
	eol_table.empty();
	if (eol_table.allocated <= reserve_size) {
		eol_table.reserve(reserve_size - eol_table.allocated);
	}

	size_t current_line_size = 1;
	for (size_t i = 0; i < get_size(); i++) {
		if (is_eol(data[i])) {
			eol_table.add(current_line_size);
			current_line_size = 1;
		} else {
			current_line_size += 1;
		}
	}
	eol_table.add(current_line_size - 1);
}

void Buffer::resize_gap(size_t new_size) {
	const size_t cursor_pos = cursor - data;
	allocated += new_size;
	data = (u8*)c_realloc(data, allocated);
	gap = data + allocated - DEFAULT_GAP_SIZE;
	gap_size = DEFAULT_GAP_SIZE;
	cursor = data + cursor_pos;
}

u8* Buffer::get_position(size_t index) {
	assert(index < get_size() + 1);

	const size_t first_data_size = (size_t)(gap - data);
	if (index <= first_data_size) {
		return data + index;
	} else {
		index -= first_data_size;
		return get_gap_end() + index;
	}
}

void Buffer::load_from_file(const char* path) {
	path = path;
	FILE* fd = fopen(path, "rb");
	if (!fd) {
		return;
	}
	fseek(fd, 0, SEEK_END);
	size_t size = ftell(fd);
	fseek(fd, 0, SEEK_SET);

	u8* buffer_data = c_new u8[size + DEFAULT_GAP_SIZE];
	data = buffer_data;
	allocated = size + DEFAULT_GAP_SIZE;

	size_t extra = 0;
	while (buffer_data != get_data_end()) {
		const u8 c = fgetc(fd);
		if (c == '\r') {
			extra += 1;
			continue;
		}
		*buffer_data = c;
		buffer_data += 1;
	}

	// fread(buffer_data, 1, size, fd);
	fclose(fd);


	gap = data + size - extra;
	gap_size = DEFAULT_GAP_SIZE + extra;

	cursor = data;

	size_t line_count = 1;

	for (size_t i = 0; i < get_size(); i++) {
		if (is_eol(data[i])) {
			line_count += 1;
		}
	}

	refresh_eol_table(line_count);
	refresh_cursor_info();
}

void Buffer::init_from_size(size_t size) {
	if (size < DEFAULT_GAP_SIZE) size = DEFAULT_GAP_SIZE;

	data = c_new u8[size];
	allocated = size;
	cursor = data;
	gap = data;
	gap_size = size;
	eol_table.add(0);

	refresh_cursor_info();
}

void Buffer::add_char(u8 c) {
	move_gap_to_cursor();

	*cursor = c;
	cursor += 1;
	gap += 1;
	gap_size -= 1;

	if (is_eol(c)) {
		const size_t line_size = eol_table[current_line_number];
		eol_table[current_line_number] = current_column_number + 1;
		eol_table.add_at_index(line_size - current_column_number, current_line_number + 1);

		current_line_number += 1;
		current_column_number = 0;
		desired_column_number = 0;
	} else {
		current_column_number += 1;
		desired_column_number += 1;
		eol_table[current_line_number] += 1;
	}

	if (gap_size == 0) {
		resize_gap(DEFAULT_GAP_SIZE);
	}
}

void Buffer::move_cursor(s32 delta) {
	assert(delta != 0);

	u8* new_cursor = cursor + delta;

	if (new_cursor < data) new_cursor = data;
	if (new_cursor > data + allocated) new_cursor = data + allocated;

	if (new_cursor > gap && new_cursor <= gap + gap_size) {
		if (delta > 0) {
			if (get_gap_end() == get_data_end() && new_cursor + gap_size >= get_data_end()) {
				new_cursor = gap;
			} else {
				new_cursor += gap_size;
			}
		} else {
			new_cursor -= gap_size;
		}
	}
	
	cursor = new_cursor;

	// @Performance: This is bad. we cant just do the easy calcs here. but using it for now
	refresh_cursor_info();
}

void Buffer::move_cursor_to(size_t pos) {
	assert(pos < get_size());
	cursor = get_position(pos);
}

void Buffer::move_cursor_line(s32 delta) {
	size_t new_line = current_line_number + delta;
	if (delta < 0 && current_line_number == 0) {
		new_line = 0;
	} else if (new_line >= eol_table.count) {
		new_line = eol_table.count - 1;
	}

	size_t line_position = 0;
	for (size_t i = 0; i < new_line; i++) {
		line_position += eol_table[i];
	}

	if (desired_column_number >= eol_table[new_line]) {
		current_column_number = eol_table[new_line] - 1;
		if (new_line == eol_table.count - 1) {
			current_column_number += 1;
		}
	} else {
		current_column_number = desired_column_number;
	}

	if (cursor == gap && get_gap_end() == get_data_end() && delta > 0) {
		return;
	}

	cursor = get_position(line_position + current_column_number);
	refresh_cursor_info(false);
}

void Buffer::remove_before_cursor() {
	move_gap_to_cursor();

	if (cursor == data) {
		return;
	}

	eol_table[current_line_number] -= 1;

	if (is_eol(*(cursor - 1))) {
		const size_t amount_on_line = eol_table[current_line_number] + 1;
		eol_table.remove(current_line_number);
		eol_table[current_line_number - 1] += amount_on_line;
	}

	cursor -= 1;
	gap -= 1;
	gap_size += 1;

	// @Hack(Colby): Slow but doing it out of laziness
	refresh_cursor_info();
}

u8* Buffer::get_line(size_t line) {
	assert(line < eol_table.count);

	size_t line_position = 0;
	for (size_t i = 0; i < line; i++) {
		line_position += eol_table[i];
	}

	return get_position(line_position);
}

void Buffer_View::input_pressed() {
	const float font_height = FONT_SIZE;
	const Vector2 size = get_size();
	const Vector2 position = get_position();

	const float cursor_y_in_buffer = (buffer->current_line_number) * font_height;
	const float cursor_y_in_view = cursor_y_in_buffer - current_scroll_y;

	const int lines_in_size = (int)(size.y / font_height);

	if (cursor_y_in_view - font_height < 0.f) {
		target_scroll_y = cursor_y_in_buffer;
		current_scroll_y = target_scroll_y;
	}
	else if (cursor_y_in_view + font_height * 2.f > size.y) {
		target_scroll_y = cursor_y_in_buffer - lines_in_size * font_height + font_height * 2.f;
		current_scroll_y = target_scroll_y;
	}
}

void Buffer_View::draw() {
	if (!buffer) return;

	const Vector2 size = get_size();

	// @NOTE(Colby): This is where we draw the text data
	{
		const Vector2 position = get_buffer_position();

		const float font_height = FONT_SIZE;
		float x = 0.f;
		float y = font_height - font.line_gap;

		font.bind();
		immediate_begin();

		const size_t lines_scrolled = (size_t)(current_scroll_y / font_height);
		size_t start_index = 0;
		for (size_t i = 0; i < lines_scrolled; i++) {
			start_index += buffer->eol_table[i];
		}

		verts_culled += start_index * 6;

		y += lines_scrolled * font_height;

#if LINE_COUNT_DEBUG
		size_t line_index = lines_scrolled;

		const char* format = "%llu: LS: %llu |";

		char out_line_size[20];
		sprintf_s(out_line_size, 20, format, line_index, buffer->eol_table[line_index]);

		x += immediate_string(out_line_size, position.x + x, position.y + y, 0xFFFF00).x;
#endif

#if !GAP_BUFFER_DEBUG
		for (size_t i = start_index; i < buffer->get_size(); i++) {
			const u8* current_position = buffer->get_position(i);
			u8 c_to_draw = *current_position;
#else 
		for (size_t i = start_index; i < buffer->allocated; i++) {
			u8 c_to_draw = buffer->data[i];
			const u8* current_position = buffer->data + i;
#endif
			Vector4 color = 0xd6b58d;


			if (current_position == buffer->cursor || (buffer->cursor == buffer->gap && current_position == buffer->get_gap_end())) {
				immediate_flush();

				const Font_Glyph& glyph = font[c_to_draw];

				if (buffer->cursor == buffer->gap || is_eol(c_to_draw) || is_whitespace(c_to_draw)) {
					draw_cursor(font.get_space_glyph(), position.x + x, position.y + y);
				}
				else {
					draw_cursor(glyph, position.x + x, position.y + y);
				}
				color = 0x052329;

				font.bind();
				immediate_begin();
			}

#if GAP_BUFFER_DEBUG
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
				if (c_to_draw == '\t') {
					const Font_Glyph& space_glyph = font.get_space_glyph();
					x += space_glyph.advance  * 4.f;
					verts_culled += 6 * 4;
					continue;
				} else if (c_to_draw == ' ') {
					x += font.get_space_glyph().advance;
					verts_culled += 6;
					continue;
				} else if (is_eol(c_to_draw) && !LINE_COUNT_DEBUG) {
					y += font_height;
					x = 0.f;
					verts_culled += 6;
					continue;
				}
			}
#else
			if (is_eol(c_to_draw) && !LINE_COUNT_DEBUG) {
				y += font_height;
				x = 0.f;
				verts_culled += 6;
				continue;
			} else if (c_to_draw == '\t') {
				const Font_Glyph& space_glyph = font.get_space_glyph();
				x += space_glyph.advance  * 4.f;
				verts_culled += 6 * 4;
				continue;
			} else if (c_to_draw == ' ') {
				x += font.get_space_glyph().advance;
				verts_culled += 6;
				continue;
			}
#endif

#if LINE_COUNT_DEBUG
			if (is_eol(c_to_draw)) {
				line_index += 1;
				x = 0.f;
				y += font_height;

				verts_culled += 6;

				char out_line_size[20];
				sprintf_s(out_line_size, 20, format, line_index, buffer->eol_table[line_index]);
				x += immediate_string(out_line_size, position.x + x, position.y + y, 0xFFFF00).x;
				continue;
			}
#endif

			assert(!is_whitespace(c_to_draw));

			const Font_Glyph& glyph = font[c_to_draw];

			if (x + glyph.advance > size.x) {
				x = 0.f;
				y += font_height;
			}

			if (get_buffer_position().y + y > size.y) {
				verts_culled += (buffer->get_size() - i) * 6;
				break;
			}

			immediate_glyph(glyph, position.x + x, position.y + y, color);
			x += glyph.advance;
		}

		{
			const u8* last_position = buffer->get_position(buffer->get_size() - 1) + 1;
			if (last_position == buffer->cursor) {
				immediate_flush();
				draw_cursor(font.get_space_glyph(), position.x + x, position.y + y);
				font.bind();
				immediate_begin();
			}
		}

#if EOF_DEBUG
		immediate_string("EOF", position.x + x, position.y + y, 0xAA00FF);
#endif

		immediate_flush();
	}

	// @NOTE(Colby): We're drawing info bar here
	{
		const Vector2 position = get_position();
		const float bar_height = FONT_SIZE + 10.f;

		const float x0 = position.x;
		const float y0 = position.y + size.y - bar_height;
		const float x1 = x0 + size.x;
		const float y1 = y0 + bar_height;
		draw_rect(x0, y0, x1, y1, 0xd6b58d);

		char output_string[1024];
		sprintf_s(output_string, 1024, " %s      LN: %llu     COL: %llu", buffer->title.data, buffer->current_line_number, buffer->current_column_number);
		String out_str = output_string;
		draw_string(out_str, x0, y0 + 5.f, 0x052329);
	}
}

void Buffer_View::tick(float delta_time) {
	current_scroll_y = Math::finterpto(current_scroll_y, target_scroll_y, delta_time, 10.f);

	if (mouse_pressed_on) {
		try_cursor_pick();
	}
}

Vector2 Buffer_View::get_size() const {
	// @HACK: until we have a better system
	return Vector2((float)OS::window_width(), (float)OS::window_height() - (FONT_SIZE + 10.f));
}

Vector2 Buffer_View::get_position() const {
	return Vector2(0.f, 0.f);
}

Vector2 Buffer_View::get_buffer_position() const {
	Vector2 result = get_position();
	result.y -= current_scroll_y;
	return result;
}

float Buffer_View::get_buffer_height() const {
	if (!buffer) {
		return 0.f;
	}
	const float font_height = FONT_SIZE;
	return buffer->eol_table.count * font_height;
}

void Buffer_View::try_cursor_pick() {
	if (!buffer) {
		return;
	}

	const Vector2 mouse_pos = OS::get_mouse_position();

	const float font_height = FONT_SIZE;

	const Vector2 position = get_buffer_position();

	float x = 0.f;
	float y = 0.f;

	size_t lines_scrolled = (size_t)((current_scroll_y + mouse_pos.y) / font_height);
	if (lines_scrolled >= buffer->eol_table.count) {
		lines_scrolled = buffer->eol_table.count - 1;
	}

	size_t start_index = 0;
	for (size_t i = 0; i < lines_scrolled; i++) {
		start_index += buffer->eol_table[i];
	}

	y += lines_scrolled * font_height;

#if LINE_COUNT_DEBUG
	size_t line_index = lines_scrolled;

	const char* format = "%llu: LS: %llu |";

	char out_line_size[20];
	sprintf_s(out_line_size, 20, format, line_index, buffer->eol_table[line_index]);

	x += get_draw_string_size(out_line_size).x;
#endif

	for (size_t i = start_index; i < buffer->get_size(); i++) {
		u8* current_position = buffer->get_position(i);
		u8 c = *current_position;

		Font_Glyph& glyph = font[c];
		const Font_Glyph& space_glyph = font.get_space_glyph();
		if (is_whitespace(c)) {
			glyph = font.get_space_glyph();
		}

		float width = glyph.advance;
		if (c == '\t') {
			width += glyph.advance * 3;
		}

		const float x0 = position.x + x;
		const float y0 = position.y + y;

		const float x1 = x0 + width;
		const float y1 = y0 + font_height;

		if (mouse_pos.x >= x0 && mouse_pos.x <= x1 && mouse_pos.y >= y0 && mouse_pos.y <= y1) {
			buffer->cursor = current_position;
			buffer->refresh_cursor_info();
			return;
		}
		x += width;

		if (is_eol(c)) {
			buffer->cursor = current_position;
			buffer->refresh_cursor_info();
			return;
		}
	}

	if (lines_scrolled == buffer->eol_table.count - 1)
	{
		u8* char_pos = buffer->get_position(buffer->get_size() - 1) + 1;

		buffer->cursor = char_pos;
		buffer->refresh_cursor_info();
		return;
	}

}

void Buffer_View::on_mouse_down(Vector2 position) {
	mouse_pressed_on = true;
}

void Buffer_View::on_mouse_up(Vector2 position) {
	mouse_pressed_on = false;
}

void Buffer_View::draw_cursor(const Font_Glyph& glyph, float x, float y) {
	const float x0 = x;
	const float y0 = y - font.ascent;
	float x1 = x0 + glyph.advance;
	const float y1 = y - font.descent;

	draw_rect(x0, y0, x1, y1, 0x81E38E);
}
