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

	operator bool() const { return data != nullptr; }

	u32& operator[](size_t index);
	u32 operator[](size_t index) const;

};

Buffer make_buffer(Buffer_ID id);

bool buffer_load_from_file(Buffer* buffer, const char* path);
void buffer_init_from_size(Buffer* buffer, size_t size);

void buffer_add_char(Buffer* buffer, u32 c);
void buffer_add_string(Buffer* buffer, const String& string);
void buffer_remove_before_cursor(Buffer* buffer);
void buffer_remove_at_cursor(Buffer* buffer);

void buffer_move_cursor_horizontal(Buffer* buffer, s64 delta);
void buffer_move_cursor_vertical(Buffer* buffer, s64 delta);

void buffer_set_cursor_from_index(Buffer* buffer, size_t index);
void buffer_refresh_cursor_info(Buffer* buffer, bool update_desired = true);



size_t buffer_get_count(const Buffer& buffer);
size_t buffer_get_cursor_index(const Buffer& buffer);
size_t buffer_get_line_index(const Buffer& buffer, size_t index);

struct Buffer_View {
	Buffer_ID buffer_id;

	u32 flags;

	float current_scroll_y = 0.f;
	float target_scroll_y = 0.f;
};

void tick_buffer_view(Buffer_View* buffer_view, float dt);
float buffer_view_get_buffer_height(const Buffer_View& buffer_view);
size_t buffer_view_pick_index(const Buffer_View& buffer_view, float x, float y, Vector2 pick_position);