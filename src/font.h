#pragma once

#include "types.h"
#include "opengl.h"

// @TODO(Colby): Replace desired font size with something that checks a script or something
#define FONT_SIZE      font_size
#define FONT_ATLAS_DIMENSION 2048
#define NUM_CHARACTERS 95

extern float font_size;

struct Font_Glyph {
	float width, height;
	float bearing_x, bearing_y;
	float advance;

	u32 x0, y0, x1, y1;
};

struct Font {
	float ascent;
	float descent;
	float line_gap;

	GLuint texture_id;
	Font_Glyph glyphs[NUM_CHARACTERS];
};

extern Font* current_font;

void font_init();
Font font_load_from_file(const char* path);
Font_Glyph font_find_glyph(Font* font, u8 c);
void font_bind(Font* font);