#pragma once

#include <ch_stl/types.h>
#include <ch_stl/filesystem.h>

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
	u32 allocated;
	u32* gap;
	usize gap_size;
	ch::Array<usize> eol_table;

	Buffer() = default;
	Buffer(Buffer_ID _id) : id(_id) {}
	Buffer copy() const;
	void free();
	CH_FORCEINLINE usize count() const { return allocated - gap_size; }

	u32& operator[](usize index);
	u32 operator[](usize index) const;

	bool load_from_path(const ch::Path& path);
};