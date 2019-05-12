#pragma once

#include "types.h"
#include "string.h"
#include "array.h"
#include "math.h"
#include "parsing.h"

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

	String language;

	u32* data;
	size_t allocated;

	u32* gap;
	size_t gap_size;

	Array<size_t> eol_table;

    Array<Syntax_Highlight> syntax;
    Array<Buf_String> macros = {};
    Array<Buf_String> types = {};

	u32& operator[](size_t index);
	u32 operator[](size_t index) const;
};

Buffer make_buffer(Buffer_ID id);

bool buffer_load_from_file(Buffer* buffer, const char* path);
void buffer_init_from_size(Buffer* buffer, size_t size);
void buffer_resize(Buffer* buffer, size_t new_gap_size);

void add_char(Buffer* buffer, u32 c, size_t index);
void remove_at_index(Buffer* buffer, size_t index);

size_t get_count(const Buffer& buffer);
size_t get_line_index(const Buffer& buffer, size_t index);

struct Buffer_View {
	Buffer_ID id;
	float current_scroll_y = 0.f;
	float target_scroll_y = 0.f;

	size_t cursor = 0;
	size_t selection = 0;

	u64 current_line_number = 0;
	u64 current_column_number = 0;
	u64 desired_column_number = 0;

	struct Editor_State* editor;
};

Buffer* get_buffer(Buffer_View* view);

void refresh_cursor_info(Buffer_View* view, bool update_desired = true);
void move_cursor_vertical(Buffer_View* view, s64 delta);
void seek_line_start(Buffer_View* view);
void seek_line_end(Buffer_View* view);
void seek_horizontal(Buffer_View* view, bool right);

void buffer_view_lost_focus(Buffer_View* view);
void buffer_view_gained_focus(Buffer_View* view);