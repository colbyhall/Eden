#include "buffer.h"
#include "parsing.h"
#include "os.h"

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

	for (size_t i = 0; buffer->data[i] != 0; i++) {
		if (is_eol(buffer->data[i])) {
			line_count += 1;
		}
	}
	buffer->line_count = line_count;
}

void buffer_init_from_size(Buffer* buffer, size_t size) {
	if (size < DEFAULT_GAP_SIZE) size = DEFAULT_GAP_SIZE;

	buffer->data = malloc(size);
	buffer->cursor = buffer->data;
	buffer->gap = buffer->data;
	buffer->gap_size = size;
	buffer->line_count = 1;
}

void buffer_add_char(Buffer* buffer, u8 c) {
	buffer_move_gap_to_cursor(buffer);

	*buffer->cursor = c;
	buffer->cursor += 1;
	buffer->gap += 1;
	buffer->gap_size -= 1;
}
