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
	eol_table.add(current_line_size);
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
	assert(index < get_size());

	const size_t first_data_size = (size_t)(gap - data);
	if (index < first_data_size) {
		return data + index;
	} else {
		index -= first_data_size;
		return get_gap_end() + index;
	}
}

void Buffer::load_from_file(const char* path) {
	path = path;
	FILE* fd = fopen(path, "r");
	if (!fd) {
		return;
	}
	fseek(fd, 0, SEEK_END);
	size_t size = ftell(fd);
	fseek(fd, 0, SEEK_SET);

	u8* buffer_data = c_new u8[size + DEFAULT_GAP_SIZE];
	fread(buffer_data, 1, size, fd);
	fclose(fd);

	data = buffer_data;
	allocated = size + DEFAULT_GAP_SIZE;

	gap = data + size;
	gap_size = DEFAULT_GAP_SIZE;

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

	if (desired_column_number > eol_table[new_line]) {
		current_column_number = eol_table[new_line] - 1;
	} else {
		current_column_number = desired_column_number;
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

#if LINE_COUNT_DEBUG
		size_t line_index = 0;

		const char* format = "%llu: LS: %llu |";

		char out_line_size[20];
		sprintf_s(out_line_size, 20, format, line_index, buffer->eol_table[line_index]);

		x += immediate_string(out_line_size, position.x + x, position.y + y, 0xFFFF00).x;
#endif
		const size_t lines_scrolled = (size_t)(current_scroll_y / font_height);
		size_t start_index = 0;
		for (size_t i = 0; i < lines_scrolled; i++) {
			start_index += buffer->eol_table[i];
		}

		y += lines_scrolled * font_height;

		for (size_t i = start_index; i < buffer->allocated; i++) {
			Vector4 color = 0xd6b58d;
			u8 c_to_draw = buffer->data[i];

			const u8* current_position = buffer->data + i;

			if (buffer->data + i == buffer->cursor) {
				immediate_flush();

				const Font_Glyph& glyph = font[buffer->data[i]];

				if (buffer->cursor == buffer->gap || is_eol(c_to_draw) || is_whitespace(c_to_draw)) {
					draw_cursor(font.get_space_glyph(), position.x + x, position.y + y);
				}
				else {
					draw_cursor(glyph, position.x + x, position.y + y);
				}
				color = 0x052329;

				immediate_begin();
				font.bind();
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
					continue;
				} else if (c_to_draw == ' ') {
					x += font.get_space_glyph().advance;
					continue;
				}
			}
#else
			if (buffer->data + i >= buffer->gap && buffer->data + i < buffer->get_gap_end()) {
				continue;
			}

			if (is_eol(c_to_draw) && !LINE_COUNT_DEBUG) {
				y += font_height;
				x = 0.f;
				continue;
			}
			else if (c_to_draw == '\t') {
				const Font_Glyph& space_glyph = font.get_space_glyph();
				x += space_glyph.advance  * 4.f;
				continue;
			}
			else if (c_to_draw == ' ') {
				x += font.get_space_glyph().advance;
				continue;
			}
#endif

#if LINE_COUNT_DEBUG
			if (is_eol(c_to_draw)) {
				line_index += 1;
				x = 0.f;
				y += font_height;

				char out_line_size[20];
				sprintf_s(out_line_size, 20, format, line_index, buffer->eol_table[line_index]);
				x += immediate_string(out_line_size, position.x + x, position.y + y, 0xFFFF00).x;
				continue;
			}
#endif

			if (buffer->cursor == buffer->gap && buffer->data + i == buffer->get_gap_end()) {
				color = 0x0f2329;
			}

			assert(!is_whitespace(c_to_draw));

			const Font_Glyph& glyph = font[c_to_draw];

			if (x + glyph.advance > size.x) {
				x = 0.f;
				y += font_height;
			}

			if (get_buffer_position().y + y > size.y) {
				break;
			}

			immediate_glyph(glyph, position.x + x, position.y + y, color);
			x += glyph.advance;
		}

		if (buffer->cursor == buffer->get_data_end()) {
			immediate_flush();
			draw_cursor(font.get_space_glyph(), position.x + x, position.y + y);
			immediate_begin();
			font.bind();
		}

#if EOF_DEBUG
		immediate_string("EOF", x, y, 0xAA00FF);
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

void Buffer_View::draw_cursor(const Font_Glyph& glyph, float x, float y) {
	const float x0 = x;
	const float y0 = y - font.ascent;
	float x1 = x0 + glyph.advance;
	const float y1 = y - font.descent;

	draw_rect(x0, y0, x1, y1, 0x81E38E);
}
