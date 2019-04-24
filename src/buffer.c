#include "buffer.h"
#include "parsing.h"
#include "os.h"

#include "stretchy_buffer.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define DEFAULT_GAP_SIZE 1024

Buffer make_buffer() {
	Buffer result;
	memset(&result, 0, sizeof(result));
	return result;
}

void destroy_buffer(Buffer* buffer) {
	if (buffer->data) {
		free(buffer->data);
		buffer->data = NULL;
	}
}

static void buffer_move_gap_to_cursor(Buffer* buffer) {
	if (buffer->cursor == buffer->gap) return;

	if (buffer->cursor < buffer->gap) {
		const size_t amount_to_move = buffer->gap - buffer->cursor;

		memcpy(buffer->cursor, buffer->gap + buffer->gap_size - amount_to_move, amount_to_move);

		buffer->gap = buffer->cursor;
	}
	else {
		const size_t amount_to_move = buffer->cursor - buffer->gap + buffer->gap_size;

		memcpy(buffer->cursor - amount_to_move, buffer->gap, amount_to_move);

		buffer->gap += amount_to_move;
		buffer->cursor = buffer->gap;
	}
}

static void buffer_update_cursor_info(Buffer* buffer) {
	u32 line_count = 1;
	u32 column_number = 0;
	for (size_t i = 0; i < buffer->allocated; i++) {
		if (buffer->data + i == buffer->cursor) {
			buffer->current_line_number = line_count;
			buffer->current_column_number = column_number;
			buffer->desired_column_number = column_number;
			return;
		}

		column_number += 1;

		if (is_eol(buffer->data[i])) {
			line_count += 1;
			column_number = 0;
		}
	}
}

void buffer_load_from_file(Buffer* buffer, const char* path) {
	buffer->path = (u8*)path;
	FILE* fd = fopen(path, "rb");
	if (!fd) {
		return;
	}
	fseek(fd, 0, SEEK_END);
	size_t size = ftell(fd);
	fseek(fd, 0, SEEK_SET);

	u8* buffer_data = (u8*)malloc(size + DEFAULT_GAP_SIZE);
	fread(buffer_data, size, 1, fd);

	fclose(fd);
	buffer->data = buffer_data;
	buffer->allocated = size + DEFAULT_GAP_SIZE;

	buffer->gap = buffer->data + size;
	buffer->gap_size = DEFAULT_GAP_SIZE;

	buffer->cursor = buffer->gap;

	*buffer->gap = 0;

	u64 line_count = 1;
	u8* last_new_line = buffer->data;

	buf_push(buffer->line_table, 0);

	for (size_t i = 0; buffer->data[i] != 0; i++) {
		if (is_eol(buffer->data[i])) {
			line_count += 1;
			
			u8* current_new_line = buffer->data + i;
			buf_push(buffer->line_table, current_new_line - last_new_line);
			last_new_line = current_new_line;
		}
	}
	buffer->line_count = line_count;

	buffer_update_cursor_info(buffer);
}

void buffer_init_from_size(Buffer* buffer, size_t size) {
	if (size < DEFAULT_GAP_SIZE) size = DEFAULT_GAP_SIZE;

	buffer->data = malloc(size);
	buffer->allocated = size;
	buffer->cursor = buffer->data;
	buffer->gap = buffer->data;
	buffer->gap_size = size;
	buffer->line_count = 1;

	buf_push(buffer->line_table, 0);

	buffer_update_cursor_info(buffer);
}

void buffer_add_char(Buffer* buffer, u8 c) {
	buffer_move_gap_to_cursor(buffer);

	*buffer->cursor = c;
	buffer->cursor += 1;
	buffer->gap += 1;
	buffer->gap_size -= 1;

	if (is_eol(c)) {
		buffer->current_line_number += 1;
		buffer->current_column_number = 0;
		buffer->desired_column_number = 0;
	} else {
		buffer->current_column_number += 1;
		buffer->desired_column_number += 1;
	}
}
