#pragma once

#include "types.h"
#include "string.h"
#include "array.h"
#include "math.h"

#define GAP_BUFFER_DEBUG 0
#define LINE_COUNT_DEBUG 0
#define EOF_DEBUG 1

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

extern struct Font font;
struct Font_Glyph;

struct Buffer {
	Buffer_ID id;

	String path;
	String title;

	bool modified;

	u8* data;
	size_t allocated;

	u8* gap;
	size_t gap_size;

	u64 current_line_number;
	u64 desired_column_number;
	u64 current_column_number;
	
	u8* cursor;

	Array<size_t> eol_table;

	Buffer(Buffer_ID id);
	Buffer() : id(0) {}

	void load_from_file(const char* path);
	void init_from_size(size_t size);

	void add_char(u8 c);

	void move_cursor(s32 delta);
	void move_cursor_to(size_t pos);
	void move_cursor_line(s32 delta);

	void remove_before_cursor();

	inline u8* get_data_end() { return data + allocated; }
	inline u8* get_gap_end() { return gap + gap_size; }
	inline size_t get_size() { return allocated - gap_size; }
	inline size_t get_line_count() const { return eol_table.count; }

	u8* get_line(size_t line);
	u8* get_position(size_t index);

	void refresh_cursor_info(bool update_desired = true);
	void refresh_eol_table(size_t reserve_size = 0);

private:

	void move_gap_to_cursor();
	void resize_gap(size_t new_size);

};

struct Buffer_View {
	Buffer* buffer = nullptr;

	float current_scroll_y = 0.f;
	float target_scroll_y = 0.f;

	void input_pressed();

	void draw();
	void tick(float delta_time);

	Vector2 get_size() const;
	Vector2 get_position() const;
	Vector2 get_buffer_position() const;
	float get_buffer_height() const;

	bool mouse_pressed_on = false;

	void try_cursor_pick();

	void on_mouse_down(Vector2 position);
	void on_mouse_up(Vector2 position);

private:
	void draw_cursor(const Font_Glyph& glyph, float x, float y);
};

