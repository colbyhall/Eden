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
	: id(id), line_count(0), modified(false), allocated(0), gap(nullptr), gap_size(0), current_line_number(0), desired_column_number(0), current_column_number(0), cursor(nullptr) {}

void Buffer::move_gap_to_cursor() {
	if (cursor == gap) return;

	if (cursor < gap) {
		const size_t amount_to_move = gap - cursor;

		memcpy(cursor, gap + gap_size - amount_to_move, amount_to_move);

		gap = cursor;
	}
	else {
		const size_t amount_to_move = cursor - gap + gap_size;

		memcpy(cursor - amount_to_move, gap, amount_to_move);

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

	u64 line_count = 1;
	u8* last_new_line = data;

	// buf_push(line_table, 0);

	for (size_t i = 0; data[i] != 0; i++) {
		if (is_eol(data[i])) {
			line_count += 1;
			
			u8* current_new_line = data + i;
			// buf_push(line_table, current_new_line - last_new_line);
			last_new_line = current_new_line;
		}
	}
	line_count = line_count;

	refresh_cursor_info();
}

void Buffer::init_from_size(size_t size) {
	if (size < DEFAULT_GAP_SIZE) size = DEFAULT_GAP_SIZE;

	data = c_new u8[size];
	allocated = size;
	cursor = data;
	gap = data;
	gap_size = size;
	line_count = 1;

	// buf_push(line_table, 0);

	refresh_cursor_info();
}

void Buffer::add_char(u8 c) {
	move_gap_to_cursor();

	*cursor = c;
	cursor += 1;
	gap += 1;
	gap_size -= 1;

	if (is_eol(c)) {
		current_line_number += 1;
		current_column_number = 0;
		desired_column_number = 0;
	} else {
		current_column_number += 1;
		desired_column_number += 1;
	}
}

void Buffer::move_cursor(s32 delta) {
	assert(delta != 0);
	// @NOTE(Colby): SUPER WIP
#if 0
	u8* new_cursor = cursor;
	const s32 abs_delta = abs(delta);
	const bool add = delta > 0;
	for (s32 i = 0; i < abs_delta; i++) {
		if (add) {
			if (new_cursor + 1 > gap) {
				new_cursor += gap_size + 1;
			} else {
				new_cursor += 1;
			}
		} else {
			if (new_cursor - 1 < gap + gap_size) {
				new_cursor -= gap_size;
			} else {
				new_cursor -= 1;
			}
		}
	}
	
	cursor = new_cursor;
#endif
}

void Buffer::move_cursor_to(size_t pos) {
	assert(pos < allocated);
	// @NOTE(Colby): Not implemented
	assert(false);
}
