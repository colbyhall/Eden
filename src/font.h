#pragma once

#include "types.h"
#include "opengl.h"

struct Font_Glyph {
	float width, height;
	float bearing_x, bearing_y;
	float advance;

	u32 x0, y0, x1, y1;
};


struct Bitmap {
	s32 width, height;
	u8* data;
};

#define NUM_CHARACTERS 95
#define FONT_SIZE      20.f
#define FONT_ATLAS_DIMENSION 2048

struct Font {

	float ascent, descent, line_gap;

	inline const Font_Glyph& get_space_glyph() const { return characters[' ' - 32]; }

	const Font_Glyph& operator[](u8 c) const;

	void bind();

	static Font load_font(const char* path);

private:
	GLuint texture_id;
	Font_Glyph characters[NUM_CHARACTERS];
};