#pragma once

#include "types.h"
#include "opengl.h"

#include <stb/stb_rect_pack.h>
// @TODO(Colby): Replace desired font size with something that checks a script or something
#include <stb/stb_truetype.h>

struct Font_Glyph {
	float width, height;
	float bearing_x, bearing_y;
	float advance;

	u32 x0, y0, x1, y1;
};

struct Font_Atlas {
    u32 w;
    u32 h;
    Font_Glyph* glyphs;
};

struct Font {
    struct stbtt_fontinfo info;

    u16 size;

    double atlas_area;

	float ascent;
	float descent;
	float line_gap;

	Font_Atlas atlases[129];
    GLuint atlas_ids[ARRAYSIZE(Font::atlases)];

    int* codepoints;

    u32 num_glyphs;
};

Font font_load_from_os(const char* file_name);
void font_pack_atlas(Font& font);
const Font_Glyph* font_find_glyph(const Font* font, u32 c);