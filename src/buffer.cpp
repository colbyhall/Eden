#include "buffer.h"
#include "parsing.h"
#include "string.h"
#include "os.h"
#include "memory.h"
#include "draw.h"
#include "font.h"
#include "input.h"
#include "editor.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

Buffer make_buffer(Buffer_ID id) {
	Buffer result;
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
	while (current_char != buffer->data + buffer->allocated) {
		const u8 c = fgetc(file);
		if (c == '\r') {
			extra += 1;
			continue;
		}
		*current_char = (u32)c;
		current_char += 1;
	}
	fclose(file);

	buffer->gap = buffer->data + file_size - extra;
	buffer->gap_size = DEFAULT_GAP_SIZE + extra;

	buffer->cursor = buffer->data;	 
	return true;
}

void buffer_init_from_size(Buffer* buffer, size_t size) {
	if (size < DEFAULT_GAP_SIZE) size = DEFAULT_GAP_SIZE;

	buffer->allocated = size;
	buffer->data = (u32*)c_alloc(size * sizeof(u32));

	buffer->gap = buffer->data;
	buffer->gap_size = size;

	buffer->cursor = buffer->data;
}

static
void buffer_move_gap_to_cursor(Buffer* buffer) {
	if (buffer->gap == buffer->cursor) return;

	// @NOTE(Colby): Make sure we're not in the gap because if we are there is a serious issue
	assert(buffer->cursor < buffer->gap || buffer->cursor >= buffer->gap + buffer->gap_size);

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
	buffer_move_gap_to_cursor(buffer);

	*buffer->cursor = c;
	buffer->cursor += 1;
	buffer->gap += 1;
	buffer->gap_size -= 1;
}

void buffer_remove_before_cursor(Buffer* buffer) {
	buffer_move_gap_to_cursor(buffer);

	buffer->cursor -= 1;
	buffer->gap -= 1;
	buffer->gap_size += 1;

}

void buffer_move_cursor_horizontal(Buffer* buffer, s64 delta) {
	u32* new_cursor = buffer->cursor + delta;

	if (new_cursor < buffer->data) new_cursor = buffer->data;
	if (new_cursor >= buffer->data + buffer->allocated) new_cursor = buffer->data + buffer->allocated - 1;

	if (new_cursor > buffer->gap && new_cursor <= buffer->gap + buffer->gap_size) {
		if (delta > 0) {
			if (buffer->gap + buffer->gap_size == buffer->data + buffer->allocated && new_cursor + buffer->gap_size >= buffer->data + buffer->allocated) {
				new_cursor = buffer->gap;
			}
			else {
				new_cursor += buffer->gap_size;
			}
		}
		else {
			new_cursor -= buffer->gap_size;
		}
	}

	buffer->cursor = new_cursor;
}

size_t buffer_get_count(const Buffer& buffer) {
	return buffer.allocated - buffer.gap_size;
}

u32 buffer_get_char_at_index(const Buffer& buffer, size_t index) {
	assert(index < buffer_get_count(buffer));

	if (buffer.data + index < buffer.gap) {
		return buffer.data[index];
	} else {
		return buffer.data[index + buffer.gap_size];
	}
}

size_t buffer_get_cursor_index(const Buffer& buffer) {
	// @TODO(Colby): Assert here if the cursor is not where it should be

	if (buffer.cursor <= buffer.gap) {
		return buffer.cursor - buffer.data;
	} else {
		return buffer.cursor - buffer.data - buffer.gap_size;
	}
}



void draw_buffer_view(const Buffer_View& buffer_view) {
	Buffer* buffer = editor_find_buffer(buffer_view.buffer_id);
	if (!buffer) {
		return;
	}

	const float starting_x = 0.f;
	const float starting_y = 0.f;

	float x = 0.f;
	float y = font.ascent;

	const float font_height = FONT_SIZE;

	font_bind(&font);
	immediate_begin();
	for (size_t i = 0; i < buffer_get_count(*buffer); i++) {	
		Color color = 0xd6b58d;
		u32 c  = buffer_get_char_at_index(*buffer, i);

		if (i == buffer_get_cursor_index(*buffer)) {
			Font_Glyph glyph = font_find_glyph(&font, c);
			
			if (is_whitespace(c)) {
				glyph = font_find_glyph(&font, ' ');
			}

			immediate_flush();
			const float x0 = x;
			const float y0 = y - font.ascent;
			const float x1 = x0 + glyph.advance;
			const float y1 = y - font.descent;
			draw_rect(x0, y0, x1, y1, 0x81E38E);
			color = 0x052329;
			font_bind(&font);
			immediate_begin();
		}

		Font_Glyph glyph = font_find_glyph(&font, ' ');

		switch (c) {
		case ' ':
			x += glyph.advance;
			continue;
		case '\t':
			x += glyph.advance * 4.f;
			continue;
		case '\n':
			x = 0.f;
			y += font_height;
			continue;
		default:
			glyph = immediate_char((u8)c, x, y, color);
			x += glyph.advance;
		}

		if (y > os_window_height()) {
			break;
		}
	}
	immediate_flush();
}

void tick_buffer_view(Buffer_View* buffer_view, float dt) {

}
