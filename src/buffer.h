#pragma once

#include <ch_stl/ch_types.h>
#include <ch_stl/ch_array.h>
#include <ch_stl/ch_string.h>

using namespace ch;

#include "string.h"
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

using Buffer_ID = usize;

struct Font_Glyph;

struct Buffer_History {
    bool removed;
    usize location;
    ch::String changes;
};

struct Buffer {
	Buffer_ID id;

    ch::String path;
    ch::String title;

    ch::String language;

	u32* data;
	usize allocated;

	u32* gap;
	usize gap_size;

	Array<usize> eol_table;

    Array<Syntax_Highlight> syntax;
    bool syntax_is_dirty;

    Array<Buffer_History> history;

    double s;
    double loc_s;
    double chars_s;

	u32& operator[](usize index);
	u32 operator[](usize index) const;
};

Buffer make_buffer(Buffer_ID id);
void destroy_buffer(Buffer* buffer);

bool buffer_load_from_file(Buffer* buffer, const char* path);
void buffer_init_from_size(Buffer* buffer, usize size);
void buffer_resize(Buffer* buffer, usize new_gap_size);

void add_char(Buffer* buffer, u32 c, usize index);
void remove_at_index(Buffer* buffer, usize index);
void remove_between(Buffer* buffer, usize first, usize last);

u32* get_index_as_cursor(Buffer* buffer, usize index);
usize get_count(const Buffer& buffer);
usize get_line_index(const Buffer& buffer, usize index);

struct Buffer_View {
	Buffer_ID id;
	float current_scroll_y = 0.f;
	float target_scroll_y = 0.f;

	usize cursor = 0;
	usize selection = 0;

	u64 current_line_number = 0;
	u64 current_column_number = 0;
    float desired_column_distance = 0;
};

Buffer* get_buffer(const Buffer_View* view);

void refresh_cursor_info(Buffer_View* view, bool update_desired = true);
void move_cursor_vertical(Buffer_View* view, s64 delta);
void seek_line_start(Buffer_View* view);
void seek_line_end(Buffer_View* view);
void seek_horizontal(Buffer_View* view, bool right);

usize pick_index(Buffer_View* view, Vector2 pos);

void buffer_view_lost_focus(Buffer_View* view);
void buffer_view_gained_focus(Buffer_View* view);

Vector2 get_view_inner_size(const Buffer_View& view);
Vector2 get_view_size(const Buffer_View& view);
Vector2 get_view_position(const Buffer_View& view);

float get_column_distance(const Buffer_View& view);

usize get_column_number_closest_to_distance(const Buffer * const buffer, usize line, float distance);
