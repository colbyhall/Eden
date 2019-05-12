#include "buffer.h"
#include "parsing.h"
#include "string.h"
#include "os.h"
#include "memory.h"
#include "draw.h"
#include "font.h"
#include "keys.h"
#include "editor.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

const float scroll_speed = 5.f;

u32& Buffer::operator[](size_t index) {
	assert(index < buffer_get_count(*this));

	if (data + index < gap) {
		return data[index];
	}
	else {
		return data[index + gap_size];
	}
}

u32 Buffer::operator[](size_t index) const{
	assert(index < buffer_get_count(*this));

	if (data + index < gap) {
		return data[index];
	}
	else {
		return data[index + gap_size];
	}
}

Buffer make_buffer(Buffer_ID id) {
    Buffer result = {0};
	result.id = id;
	return result;
}

bool buffer_load_from_file(Buffer* buffer, const char* path) {
	FILE* file = fopen(path, "rb");
	if (!file) {
		return false;
	}
	
	buffer->path = path;
	fseek(file, 0, SEEK_END);
	const size_t file_size = ftell(file);
	fseek(file, 0, SEEK_SET);

	buffer->allocated = file_size + DEFAULT_GAP_SIZE;
	buffer->data = (u32*)c_alloc(buffer->allocated * sizeof(u32));

	size_t extra = 0;
	u32* current_char = buffer->data;
	size_t line_size = 0;
	while (current_char != buffer->data + file_size) {
		const u8 c = fgetc(file);
		if (c == '\r') {
			extra += 1;
			continue;
		} 
		*current_char = (u32)c;
		current_char += 1;
		line_size += 1;
		if (c == '\n'){
			array_add(&buffer->eol_table, line_size);
			line_size = 0;
		}
	}
    // @Cleanup: why does + 1 make things work here?
    array_add(&buffer->eol_table, line_size - extra + 1);
	fclose(file);

	buffer->gap = buffer->data + file_size - extra;
	buffer->gap_size = DEFAULT_GAP_SIZE + extra;

	buffer->cursor = buffer->data;	 
	buffer_refresh_cursor_info(buffer);

	// @NOTE(Colby): Basic file name parsing
	for (size_t i = buffer->path.count - 1; i >= 0; i--) {
		if (buffer->path[i] == '.') {
			buffer->language.data = buffer->path.data + i + 1;
			buffer->language.count = buffer->path.count - i - 1;
			buffer->language.allocated = buffer->language.count;
		}

		if (buffer->path[i] == '/' || buffer->path[i] == '\\') {
			buffer->title.data = buffer->path.data + i + 1;
			buffer->title.count = buffer->path.count - i - 1;
			break;
		}
	}

    // @Temporary
    parse_syntax(buffer);
	return true;
}

void buffer_init_from_size(Buffer* buffer, size_t size) {
	if (size < DEFAULT_GAP_SIZE) size = DEFAULT_GAP_SIZE;

	buffer->allocated = size;
	buffer->data = (u32*)c_alloc(size * sizeof(u32));

	buffer->gap = buffer->data;
	buffer->gap_size = size;

	buffer->cursor = buffer->data;

	buffer->current_column_number = 0;
	buffer->desired_column_number = 0;
	buffer->current_line_number = 0;
	array_add(&buffer->eol_table, (size_t)0);

	buffer->language = "cpp";

    // @Temporary
    parse_syntax(buffer);
}

void buffer_resize(Buffer* buffer, size_t new_gap_size) {
	assert(buffer->gap_size == 0);
	size_t old_size = buffer->allocated;
	size_t new_size = old_size + new_gap_size;

	size_t cursor_as_index_into_buffer = buffer_get_cursor_index(*buffer);

	u32 *old_data = buffer->data;
	u32 *new_data = (u32 *)c_realloc(old_data, new_size * sizeof(u32));
	if (!new_data) {
		assert(0); // @Todo: handle reallocation failures.
	}
	buffer->data = new_data;
	buffer->cursor = (new_data + cursor_as_index_into_buffer);
	buffer->allocated = new_size;
	buffer->gap = (buffer->data + old_size);
	buffer->gap_size = new_gap_size;

	if (!old_data) { // bit of a @Hack.
		assert(buffer->eol_table.count == 0);
		array_add(&buffer->eol_table, (size_t)0);
	}
}

static void buffer_assert_cursor_outside_gap(Buffer *buffer) {
	if (buffer->gap == buffer->cursor) return;
    // @NOTE(Colby): Make sure we're not in the gap because if we are there is a serious issue
	assert(buffer->cursor < buffer->gap || buffer->cursor >= buffer->gap + buffer->gap_size);
}

static
void buffer_move_gap_to_cursor(Buffer* buffer) {
	if (buffer->gap == buffer->cursor) return;

	buffer_assert_cursor_outside_gap(buffer);

	if (buffer->cursor < buffer->gap) {
		const size_t amount_to_move = buffer->gap - buffer->cursor;

		memcpy(buffer->gap + buffer->gap_size - amount_to_move, buffer->cursor, amount_to_move * sizeof(u32));

		buffer->gap = buffer->cursor;
	} else {
		const size_t amount_to_move = buffer->cursor - (buffer->gap + buffer->gap_size);

		memcpy(buffer->gap, buffer->gap + buffer->gap_size, amount_to_move * sizeof(u32));

		buffer->gap += amount_to_move;
		buffer->cursor = buffer->gap;
	}
}

void buffer_add_char(Buffer* buffer, u32 c) {
    if (buffer->gap_size <= 0) { // @Refactor into buffer_resize
		buffer_resize(buffer, DEFAULT_GAP_SIZE);
    }

	buffer_move_gap_to_cursor(buffer);

	*buffer->cursor = c;
	buffer->cursor += 1;
	buffer->gap += 1;
	buffer->gap_size -= 1;

	if (is_eol(c)) {
		const size_t line_size = buffer->eol_table[buffer->current_line_number];
		buffer->eol_table[buffer->current_line_number] = buffer->current_column_number + 1;
		array_add_at_index(&buffer->eol_table, line_size - buffer->current_column_number, buffer->current_line_number + 1);
		
		buffer->current_line_number += 1;
		buffer->current_column_number = 0;
		buffer->desired_column_number = 0;
	} else {
		buffer->current_column_number += 1;
		buffer->desired_column_number += 1;
		buffer->eol_table[buffer->current_line_number] += 1;
	}

    buffer_refresh_cursor_info(buffer, true);

	// @TODO(Colby): Pass in actual language
	parse_syntax(buffer);
}

void buffer_add_string(Buffer* buffer, const String& string) {
	for (size_t i = 0; i < string.count; i++) {
		// @TODO(Colby): This is slow because we parse syntax after ever char
		buffer_add_char(buffer, string[i]);
	}
}

void buffer_remove_before_cursor(Buffer* buffer) {
	if (buffer->cursor == buffer->data) {
		return;
	}

	buffer_move_gap_to_cursor(buffer);

	buffer->eol_table[buffer->current_line_number] -= 1;

	const u32 c = *(buffer->cursor - 1);
	if (is_eol(c)) {
		const size_t amount_on_line = buffer->eol_table[buffer->current_line_number];
		array_remove_index(&buffer->eol_table, buffer->current_line_number);
		buffer->eol_table[buffer->current_line_number - 1] += amount_on_line;
	}

	buffer->cursor -= 1;
	buffer->gap -= 1;
	buffer->gap_size += 1;

	buffer_refresh_cursor_info(buffer, true);

	parse_syntax(buffer);
}

void buffer_remove_at_cursor(Buffer* buffer) {
	if (buffer_get_cursor_index(*buffer) >= buffer_get_count(*buffer)) {
		return;
	}

	buffer_move_gap_to_cursor(buffer);

	buffer->eol_table[buffer->current_line_number] -= 1;

	const size_t cursor_index = buffer_get_cursor_index(*buffer);
	const u32 c = (*buffer)[cursor_index];
	if (is_eol(c)) {
		const size_t amount_on_line = buffer->eol_table[buffer->current_line_number];
		array_remove_index(&buffer->eol_table, buffer->current_line_number);
		buffer->eol_table[buffer->current_line_number] += amount_on_line;
	}

	buffer->gap_size += 1;

	buffer_refresh_cursor_info(buffer, true);
	parse_syntax(buffer);
}

void buffer_move_cursor_horizontal(Buffer* buffer, s64 delta) {
	assert(delta != 0);

    buffer_assert_cursor_outside_gap(buffer);

	u32* new_cursor = buffer->cursor + delta;

	if (new_cursor < buffer->data) new_cursor = buffer->data;
	if (new_cursor > buffer->data + buffer->allocated) new_cursor = buffer->data + buffer->allocated;

	if (new_cursor > buffer->gap && new_cursor <= buffer->gap + buffer->gap_size) {
		if (delta > 0) {
			if (buffer->gap + buffer->gap_size == buffer->data + buffer->allocated && new_cursor + buffer->gap_size >= buffer->data + buffer->allocated) {
				new_cursor = buffer->gap;
			} else {
				new_cursor += buffer->gap_size;
			}
		} else {
			new_cursor -= buffer->gap_size;
		}
	}

	buffer->cursor = new_cursor;
	buffer_refresh_cursor_info(buffer);
}

void buffer_move_cursor_vertical(Buffer* buffer, s64 delta) {
	const size_t buffer_count = buffer_get_count(*buffer);
	if (buffer_count == 0) {
		return;
	}

    buffer_assert_cursor_outside_gap(buffer);

	size_t new_line = buffer->current_line_number + delta;
	if (delta < 0 && buffer->current_line_number == 0) {
		new_line = 0;
	} else if (new_line >= buffer->eol_table.count) {
		new_line = buffer->eol_table.count - 1;
	}

	const size_t line_index = buffer_get_line_index(*buffer, new_line);

	size_t new_cursor_index = line_index;
	if (buffer->eol_table[new_line] <= buffer->desired_column_number) {
		new_cursor_index += buffer->eol_table[new_line] - 1;
	} else {
		new_cursor_index += buffer->desired_column_number;
	}

	buffer_set_cursor_from_index(buffer, new_cursor_index);
	buffer_refresh_cursor_info(buffer, false);
}

void buffer_seek_horizontal(Buffer* buffer, bool right) {
	const size_t buffer_size = buffer_get_count(*buffer);
	size_t cursor_index = buffer_get_cursor_index(*buffer);

	if (right) {
		if (cursor_index == buffer_size) {
			return;
		}
		cursor_index += 1;
	} else {
		if (cursor_index == 0) {
			return;
		}
		cursor_index -= 1;
	}

	while (cursor_index > 0 && cursor_index < buffer_size) {
		const u32 c = (*buffer)[cursor_index];

		if (is_whitespace(c) || is_symbol(c)) break;

		if (right) {
			cursor_index += 1;
		} else {
			cursor_index -= 1;
		}
	}
	buffer_set_cursor_from_index(buffer, cursor_index);
	buffer_refresh_cursor_info(buffer, true);
}

void buffer_seek_line_begin(Buffer *buffer) {
    // @Todo: indent-sensitive home seeking
    size_t cursor_index = buffer_get_cursor_index(*buffer);
    cursor_index -= buffer->current_column_number;
    buffer_set_cursor_from_index(buffer, cursor_index);
    buffer_refresh_cursor_info(buffer);
}
void buffer_seek_line_end(Buffer *buffer) {
    size_t cursor_index = buffer_get_cursor_index(*buffer);
    if (buffer->eol_table.count) {
        const size_t line_length = buffer->eol_table[buffer->current_line_number];
        if (!line_length) {
            assert(buffer->current_column_number == 0);
        } else {
            cursor_index += line_length - buffer->current_column_number - 1;
        }
    }
    buffer_set_cursor_from_index(buffer, cursor_index);
    buffer_refresh_cursor_info(buffer);
}

void buffer_set_cursor_from_index(Buffer* buffer, size_t index) {
	if (index <= (size_t)(buffer->gap - buffer->data)) {
		buffer->cursor = buffer->data + index;
	} else {
		buffer->cursor = buffer->data + index + buffer->gap_size;
	}
}

void buffer_refresh_cursor_info(Buffer* buffer, bool update_desired) {
	buffer->current_line_number = 0;
	buffer->current_column_number = 0;
	for (size_t i = 0; i < buffer_get_cursor_index(*buffer); i++) {
		const u32 c = (*buffer)[i];

		buffer->current_column_number += 1;

		if (is_eol(c)) {
			buffer->current_line_number += 1;
			buffer->current_column_number = 0;
		}
	}

	if (update_desired) {
		buffer->desired_column_number = buffer->current_column_number;
	}
}

size_t buffer_get_count(const Buffer& buffer) {
	return buffer.allocated - buffer.gap_size;
}

size_t buffer_get_cursor_index(const Buffer& buffer) {
	assert(buffer.cursor <= buffer.gap || buffer.cursor > buffer.gap + buffer.gap_size);
	assert(buffer.cursor >= buffer.data && buffer.cursor <= buffer.data + buffer.allocated);

	if (buffer.cursor <= buffer.gap) {
		return buffer.cursor - buffer.data;
	} else {
		return buffer.cursor - buffer.data - buffer.gap_size;
	}
}

size_t buffer_get_line_index(const Buffer& buffer, size_t index) {
	assert(index < buffer.eol_table.count);
	
	size_t result = 0;
	for (size_t i = 0; i < index; i++) {
		result += buffer.eol_table[i];
	}

	return result;
}
