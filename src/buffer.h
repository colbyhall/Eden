#pragma once

#include <ch_stl/types.h>
#include <ch_stl/filesystem.h>
#include <ch_stl/gap_buffer.h>
#include "draw.h"

using Buffer_ID = usize;

struct Buffer {
	Buffer_ID id;
	ch::Gap_Buffer<u32> gap_buffer;
	ch::Path full_path;
	ch::Array<usize> eol_table;

	Buffer();
	Buffer(Buffer_ID _id);
	Buffer copy() const;
	void free();
};

// @NOTE(CHall): This is generally for debug purposes 
void draw_buffer(const Buffer& buffer, const Font& font, f32 x, f32 y, const ch::Color& color, f32 z_index = 9.f);