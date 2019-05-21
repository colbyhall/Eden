#pragma once

#include "types.h"
#include "opengl.h"

// @TODO(Colby): Replace desired font size with something that checks a script or something
#include <stb/stb_truetype.h>

struct Font_Glyph {
	float width, height;
	float bearing_x, bearing_y;
	float advance;

	u32 x0, y0, x1, y1;
};

struct Font {
    struct stbtt_fontinfo info;

    float size;

	float ascent;
	float descent;
	float line_gap;

	GLuint texture_id;
    u32 atlas_w;
    u32 atlas_h;

    int* codepoints;

    u32 num_glyphs;
	Font_Glyph *glyphs;
};

Font font_load_from_os(const char* file_name);
void font_pack_atlas(Font& font);
const Font_Glyph* font_find_glyph(const Font* font, u32 c);