#pragma once

#include "types.h"
#include "string.h"
#include "array.h"
#include "math.h"

#define GAP_BUFFER_DEBUG 0
#define LINE_COUNT_DEBUG 0
#define EOF_DEBUG 0

#define DEFAULT_GAP_SIZE 1024

/* ---- GAP BUFFER ----
	* = Cursor
	| = Start of Gap and Cursor
	[ = Start of Gap
	] = End of Gap
	Example:
	How many woodchucks |                      ]
	How many woodchucks would |                ]
	How many woodchucks*would  [                ]
	How many woodchucks|             ] would
*/

using Buffer_ID = size_t;

extern struct Font font;
struct Font_Glyph;

struct Buffer {
	Buffer_ID id;

	String path;
	String title;

	u32* data;
	size_t allocated;

	u32* gap;
	size_t gap_size;

	u64 current_line_number;
	u64 current_column_number;
	u64 desired_column_number;
	
	u32* cursor;

	Array<size_t> eol_table;
};

Buffer make_buffer(Buffer_ID id);

bool buffer_load_from_file(Buffer* buffer, const char* path);
void buffer_init_from_size(Buffer* buffer, size_t size);

void buffer_add_char(Buffer* buffer, u32 c);
void buffer_remove_before_cursor(Buffer* buffer);

void buffer_move_cursor_horizontal(Buffer* buffer, s64 delta);

size_t buffer_get_count(const Buffer& buffer);
u32 buffer_get_char_at_index(const Buffer& buffer, size_t index);
size_t buffer_get_cursor_index(const Buffer& buffer);

struct Buffer_View {
	Buffer_ID buffer_id;

	float current_scroll_y = 0.f;
	float target_scroll_y = 0.f;
};

void draw_buffer_view(const Buffer_View& buffer_view);
void tick_buffer_view(Buffer_View* buffer_view, float dt);