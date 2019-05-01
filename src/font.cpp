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

Font* current_font = nullptr;

Font font_load_from_file(const char* path) {
	String font_data = os_load_file_into_memory(path);
	assert(font_data.data != NULL);

	Font result;

	stbtt_fontinfo info;
	stbtt_InitFont(&info, font_data.data, stbtt_GetFontOffsetForIndex(font_data.data, 0));

	Bitmap atlas;
	atlas.width = atlas.height = FONT_ATLAS_DIMENSION;
	atlas.data = (u8*) c_alloc(atlas.width * atlas.height);

	stbtt_pack_context pc;
	stbtt_packedchar pdata[NUM_CHARACTERS];
	stbtt_pack_range pr;

	stbtt_PackBegin(&pc, atlas.data, atlas.height, atlas.height, 0, 1, NULL);

	pr.chardata_for_range = pdata;
	pr.array_of_unicode_codepoints = NULL;
	pr.first_unicode_codepoint_in_range = 32;
	pr.num_chars = NUM_CHARACTERS;
	pr.font_size = FONT_SIZE;

	const u32 h_oversample = 8;
	const u32 v_oversample = 8;

	stbtt_PackSetOversampling(&pc, h_oversample, v_oversample);
	stbtt_PackFontRanges(&pc, font_data.data, 0, &pr, 1);
	stbtt_PackEnd(&pc);

	glGenTextures(1, &result.texture_id);
	glBindTexture(GL_TEXTURE_2D, result.texture_id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, atlas.width, atlas.height, 0, GL_RED, GL_UNSIGNED_BYTE, atlas.data);

	for (int i = 0; i < NUM_CHARACTERS; i++) {
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

	c_free(font_data.data);

	return result;
}

Font_Glyph font_find_glyph(Font* font, u8 c) {
	return font->glyphs[c - 32];
}

void font_bind(Font* font) {
	shader_bind(&font_shader);
	refresh_transformation();
	glUniform1i(font_shader.texture_loc, 0);

	glBindTexture(GL_TEXTURE_2D, font->texture_id);
	glActiveTexture(GL_TEXTURE0);

	current_font = font;
}
