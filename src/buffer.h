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

using Buffer_ID = size_t;

struct Buffer {
	Buffer_ID id;

	String path;
	String title;

	u64 line_count;
	bool modified;

	u8* data;
	size_t allocated;

	u8* gap;
	size_t gap_size;

	u64 current_line_number;
	u64 desired_column_number;
	u64 current_column_number;

	u8* cursor;

	Buffer(Buffer_ID id);
	Buffer() : id(0) {}

	void load_from_file(const char* path);
	void init_from_size(size_t size);

	void add_char(u8 c);

private:
	void move_gap_to_cursor();
	void refresh_cursor_info();
};

