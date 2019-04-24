#pragma once

#include "types.h"
#include "string.h"

/* ---- GAP BUFFER ----
	* = Cursor
	[ = Start of Gap
	] = End of Gap
	Example:
	How many woodchucks *[                      ]
	How many woodchucks would *[                ]
	How many woodchucks*would  [                ]
	How many woodchucks*[             ] would
*/

typedef struct Buffer {
	u8* path;
	String title;

	u64 line_count;
	bool modified;

	u8* data;
	size_t allocated;

	u8* gap;
	size_t gap_size;

	// @NOTE(Colby): Stretchy Buffer that keeps the delta between lines
	u32* line_table;

	u64 current_line_number;
	u64 desired_column_number;
	u64 current_column_number;

	u8* cursor;
} Buffer;

Buffer make_buffer();
void destroy_buffer(Buffer* buffer);
void buffer_load_from_file(Buffer* buffer, const char* path);
void buffer_init_from_size(Buffer* buffer, size_t size);
void buffer_add_char(Buffer* buffer, u8 c);