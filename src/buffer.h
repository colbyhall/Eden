#pragma once

#include <ch_stl/types.h>
#include <ch_stl/filesystem.h>
#include "draw.h"

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

struct Buffer {
	Buffer_ID id;
	ch::Path full_path;
	u32* data;
	usize allocated;
	u32* gap;
	usize gap_size;
	ch::Array<usize> eol_table;

	Buffer();
	Buffer(Buffer_ID _id);
	Buffer copy() const;
	void free();
	CH_FORCEINLINE usize count() const { return allocated - gap_size; }

	u32& operator[](usize index);
	u32 operator[](usize index) const;

	bool load_from_path(const ch::Path& path);
	bool init_from_size(usize size);

	void resize(usize new_gap_size);
	void move_gap_to_index(usize index);
	u32* get_index_as_cursor(usize index);
	void add_char(u32 c, usize index);
	void remove_at_index(usize index);
	void remove_between(usize index, usize count);

	usize get_line_index(usize index) const;
	usize get_line_from_index(usize index) const;
};

// @NOTE(CHall): This is generally for debug purposes 
void draw_buffer(const Buffer& buffer, const Font& font, f32 x, f32 y, const ch::Color& color, f32 z_index = 9.f);