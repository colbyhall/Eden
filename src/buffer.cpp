#include "buffer.h"
#include "parsing.h"
#include "string.h"
#include "os.h"
#include "memory.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define DEFAULT_GAP_SIZE 1024

Buffer::Buffer(Buffer_ID id) 
	: id(id), modified(false), allocated(0), gap(nullptr), gap_size(0), current_line_number(0), desired_column_number(0), current_column_number(0), cursor(nullptr) {}

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

void Buffer::refresh_cursor_info() {
	u32 line_count = 1;
	u32 column_number = 0;
	for (size_t i = 0; i < allocated; i++) {
		if (data + i == cursor) {
			current_line_number = line_count;
			current_column_number = column_number;
			desired_column_number = column_number;
			return;
		}

		column_number += 1;

		if (is_eol(data[i])) {
			line_count += 1;
			column_number = 0;
		}
	}
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
	size_t i = 0;
	while (i < allocated && index != 0) {
		if (data + i == gap) {
			i += gap_size;
		} else {
			i += 1;
		}
		index -= 1;
	}

	return data + i;
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
	fread(buffer_data, size, 1, fd);

	fclose(fd);
	data = buffer_data;
	allocated = size + DEFAULT_GAP_SIZE;

	gap = data + size;
	gap_size = DEFAULT_GAP_SIZE;

	cursor = gap;

	*gap = 0;

	u8* last_new_line = data;

	eol_table.add(0);

	for (size_t i = 0; data[i] != 0; i++) {
		if (is_eol(data[i])) {
			u8* current_new_line = data + i;
			eol_table.add(current_new_line - last_new_line);
			last_new_line = current_new_line;
		}
	}

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
		const size_t line_size = eol_table[current_line_number - 1];
		eol_table[current_line_number - 1] = current_column_number;
		eol_table.add(line_size - current_column_number);
		current_line_number += 1;
		current_column_number = 0;
		desired_column_number = 0;
	} else {
		current_column_number += 1;
		desired_column_number += 1;
		eol_table[current_line_number - 1] += 1;
	}

	if (gap_size == 0) {
		resize_gap(DEFAULT_GAP_SIZE);
	}
}

void Buffer::move_cursor(s32 delta) {
	assert(delta != 0);

	u8* new_cursor = cursor + delta;

	if (new_cursor < data) new_cursor = data;

	if (new_cursor > gap && new_cursor < gap + gap_size) {
		if (delta > 0) {
			if (get_gap_end() == get_data_end() && new_cursor + gap_size >= get_data_end()) {
				new_cursor = gap;
			} else {
				new_cursor += gap_size - 1;
			}
		} else {
			new_cursor -= gap_size - 1;
		}
	}
	
	cursor = new_cursor;

	// @Performance: This is bad. we cant just do the easy calcs here. but using it for now
	refresh_cursor_info();
}

void Buffer::move_cursor_to(size_t pos) {
	assert(pos < allocated);
	cursor = get_position(pos);
}

void Buffer::move_cursor_line(s32 delta) {
	size_t desired_line = current_line_number + delta;

	if (desired_line > get_line_count()) desired_line = get_line_count() - 1;


	cursor = get_line(desired_line);
	const size_t line_length = eol_table[desired_line];
	current_line_number = desired_line + 1;
	if (line_length < current_column_number) current_column_number = line_length;
	cursor += current_column_number;
}

void Buffer::remove_before_cursor() {
	move_gap_to_cursor();

	if (cursor == data) {
		return;
	}
	cursor -= 1;
	gap -= 1;
	gap_size += 1;
}

u8* Buffer::get_line(size_t line) {
	assert(line < eol_table.count);

	size_t line_position = 0;
	for (size_t i = 0; i < line; i++) {
		line_position += eol_table[i];
	}

	return get_position(line_position);
}
