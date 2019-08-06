#pragma once

#include <ch_stl/opengl.h>
#include <ch_stl/math.h>

#include <stb/stb_truetype.h>

struct stbtt_fontinfo;

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
	stbtt_fontinfo info;

	u16 size;

	f64 atlas_area;

	f32 ascent;
	f32 descent;
	f32 line_gap;

	Font_Atlas atlases[129];
	GLuint atlas_ids[ARRAYSIZE(Font::atlases)];

	s32* codepoints;
	u32 num_glyphs;

	CH_FORCEINLINE void free() {
		ch_delete codepoints;
	}

	const Font_Glyph* operator[](u32 c) const;

	void pack_atlas();
	void bind();
};

bool load_font_from_path(const ch::Path& path, Font* out_font);

bool draw_init();
void draw_begin();
void draw_end();

void refresh_shader_transform();
void render_right_handed();

void immediate_begin();
void immediate_flush();

void immediate_vertex(f32 x, f32 y, const ch::Color& color, ch::Vector2 uv = 0.f, f32 z_index = 9.f);

void immediate_quad(f32 x0, f32 y0, f32 x1, f32 y1, const ch::Color& color, f32 z_index = 9.f);
CH_FORCEINLINE void draw_quad(f32 x0, f32 y0, f32 x1, f32 y1, const ch::Color& color, f32 z_index = 9.f) {
	immediate_begin();
	immediate_quad(x0, y0, x1, y1, color, z_index);
	immediate_flush();
}

void immediate_glyph(const Font_Glyph& glyph, const Font& font, f32 x, f32 y, const ch::Color& color, f32 z_index = 9.f);
CH_FORCEINLINE void draw_glyph(const Font_Glyph& glyph, const Font& font, f32 x, f32 y, const ch::Color& color, f32 z_index = 9.f) {
	immediate_begin();
	immediate_glyph(glyph, font, x, y, color, z_index);
	immediate_flush();
}

const Font_Glyph* immediate_char(const u32 c, const Font& font, f32 x, f32 y, const ch::Color& color, f32 z_index = 9.f);
CH_FORCEINLINE void draw_char(const u32 c, const Font& font, f32 x, f32 y, const ch::Color& color, f32 z_index = 9.f) {
	immediate_begin();
	immediate_char(c, font, x, y, color, z_index);
	immediate_flush();
}