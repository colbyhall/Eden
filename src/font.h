#pragma once

#include "types.h"
#include "opengl.h"

typedef struct Font_Glyph {
	float width, height;
	float bearing_x, bearing_y;
	float advance;

	u32 x0, y0, x1, y1;
} Font_Glyph;


typedef struct Bitmap {
	s32 width, height;
	u8* data;
} Bitmap;

#define NUM_CHARACTERS 95
#define FONT_SIZE      20.f
#define FONT_ATLAS_DIMENSION 2048

typedef struct Font {

	GLuint texture_id;

	float ascent, descent, line_gap;

	Font_Glyph characters[NUM_CHARACTERS];

} Font;

Font load_font(const char* path);