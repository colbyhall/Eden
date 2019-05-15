#include "font.h"
#include "os.h"
#include "memory.h"
#include "draw.h"

#include <assert.h>
#include <stdlib.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb/stb_truetype.h>

struct Bitmap {
	s32 width, height;
	u8* data;
};

float font_size = 20.f;

Font* current_font = nullptr;

u32 round_up_pow2(u32 x) {
    x--;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x++;
    return x;
}

Font font_load_from_os(const char* file_name) {
	const String current_path = os_get_path();
	os_set_path_to_fonts();

	String font_data = os_load_file_into_memory(file_name);
	assert(font_data.data != NULL);

	Font result;
    
	stbtt_fontinfo info;
	stbtt_InitFont(&info, font_data.data, stbtt_GetFontOffsetForIndex(font_data.data, 0));
                                                                                          
    result.num_glyphs = info.numGlyphs;// @Temporary: info is opaque, but we are peeking :)
    result.glyphs = (Font_Glyph*) c_alloc(result.num_glyphs * sizeof(Font_Glyph));

	const u32 h_oversample = 2; // @NOTE(Phillip): On a low DPI display this looks almost as good to me.
	const u32 v_oversample = 2; //                 The jump from 1x to 4x is way bigger than 4x to 64x.

	Bitmap atlas;

    u32 atlas_dimension = ((u32)sqrtf((float)result.num_glyphs) / 2 * (int)(font_size + 0.5f));

	atlas.width = round_up_pow2(atlas_dimension) * h_oversample;
    atlas.height = round_up_pow2(atlas_dimension) * v_oversample;
    result.atlas_w = atlas.width;
    result.atlas_h = atlas.height;

	atlas.data = (u8*) c_alloc(atlas.width * atlas.height);

    stbtt_pack_context pc;
	stbtt_packedchar* pdata = (stbtt_packedchar*) c_alloc(result.num_glyphs * sizeof(stbtt_packedchar));
    stbtt_pack_range pr;

	stbtt_PackBegin(&pc, atlas.data, atlas.width, atlas.height, 0, 1, NULL);

    int* codepoints = (int*) c_alloc(result.num_glyphs * sizeof(int));
    memset(codepoints, 0, result.num_glyphs * sizeof(int));
    {
        u32 glyphs_found = 0;
        // Ask STBTT for the glyph indices.
        // @Temporary: linearly search the codepoint space because STBTT doesn't expose CP->glyph idx;
        //             later we will parse the ttf file in a similar way to STBTT.
        //             Linear search is exactly 17 times slower than parsing for 65536 glyphs.
        for (int codepoint = 0; codepoint < 0x110000; codepoint++) {
            int idx = stbtt_FindGlyphIndex(&info, codepoint);
            if (idx <= 0) continue;
            glyphs_found++;
            codepoints[idx] = codepoint;
        }
        //if (glyphs_found < result.num_glyphs) {
        //
        //}
    }
    
	pr.chardata_for_range = pdata;
	pr.array_of_unicode_codepoints = codepoints;
    pr.first_unicode_codepoint_in_range = 0;
	pr.num_chars = result.num_glyphs;
	pr.font_size = FONT_SIZE;

    stbtt_PackSetSkipMissingCodepoints(&pc, 1);
	stbtt_PackSetOversampling(&pc, h_oversample, v_oversample);
	stbtt_PackFontRanges(&pc, font_data.data, 0, &pr, 1);
	stbtt_PackEnd(&pc);

	glGenTextures(1, &result.texture_id);
	glBindTexture(GL_TEXTURE_2D, result.texture_id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, atlas.width, atlas.height, 0, GL_RED, GL_UNSIGNED_BYTE, atlas.data);

	for (u32 i = 0; i < result.num_glyphs; i++) {
		Font_Glyph* glyph = &result.glyphs[i];

		glyph->x0 = pdata[i].x0;
		glyph->x1 = pdata[i].x1;
		glyph->y0 = pdata[i].y0;
		glyph->y1 = pdata[i].y1;

		glyph->width = ((f32)pdata[i].x1 - pdata[i].x0) / (float)h_oversample;
		glyph->height = ((f32)pdata[i].y1 - pdata[i].y0) / (float)v_oversample;
		glyph->bearing_x = pdata[i].xoff;
		glyph->bearing_y = pdata[i].yoff;
		glyph->advance = pdata[i].xadvance;
	}

	glBindTexture(GL_TEXTURE_2D, 0);


	int ascent, descent, line_gap;
	stbtt_GetFontVMetrics(&info, &ascent, &descent, &line_gap);

	const float font_scale = stbtt_ScaleForPixelHeight(&info, FONT_SIZE);
	result.ascent = (float)ascent * font_scale;
	result.descent = (float)descent * font_scale;
	result.line_gap = (float)line_gap * font_scale;

    result.info = info;
	// Don't free the data, since result.info needs it.

    c_free(codepoints);

	os_set_path(current_path);

	bind_font(&result);

	return result;
}

const Font_Glyph* font_find_glyph(const Font* font, u32 c) {
    int idx = stbtt_FindGlyphIndex(&font->info, c);
    if (idx > 0) {
        assert((u32)idx < font->num_glyphs);
        return &font->glyphs[idx];
    } else {
        return nullptr;
    }
}
