#pragma once

#include <ch_stl/opengl.h>
#include <ch_stl/math.h>

#include <stb/stb_truetype.h>

struct Font_Glyph {
	f32 width, height;
	f32 bearing_x, bearing_y;
	f32 advance;

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

	static const usize num_atlases = 129;

	Font_Atlas atlases[num_atlases];
	GLuint atlas_ids[num_atlases];

	s32* codepoints;
	u32 num_glyphs;

	CH_FORCEINLINE void free() {
		ch_delete codepoints;
	}

	const Font_Glyph* operator[](u32 c) const;

	void pack_atlas();
	void bind() const;
};

bool load_font_from_path(const ch::Path& path, Font* out_font);

void init_draw();

void refresh_shader_transform();
void render_right_handed();

void frame_begin();
void frame_end();

void imm_begin();
void imm_flush();

void imm_vertex(f32 x, f32 y, const ch::Color& color, ch::Vector2 uv = 0.f, f32 z_index = 9.f);

void imm_quad(f32 x0, f32 y0, f32 x1, f32 y1, const ch::Color& color, f32 z_index = 9.f);
CH_FORCEINLINE void draw_quad(f32 x0, f32 y0, f32 x1, f32 y1, const ch::Color& color, f32 z_index = 9.f) {
	imm_begin();
	imm_quad(x0, y0, x1, y1, color, z_index);
	imm_flush();
}

void imm_glyph(const Font_Glyph* glyph, const Font& font, f32 x, f32 y, const ch::Color& color, f32 z_index = 9.f);
CH_FORCEINLINE void draw_glyph(const Font_Glyph* glyph, const Font& font, f32 x, f32 y, const ch::Color& color, f32 z_index = 9.f) {
	imm_begin();
	imm_glyph(glyph, font, x, y, color, z_index);
	imm_flush();
}

const Font_Glyph* imm_char(const u32 c, const Font& font, f32 x, f32 y, const ch::Color& color, f32 z_index = 9.f);
CH_FORCEINLINE void draw_char(const u32 c, const Font& font, f32 x, f32 y, const ch::Color& color, f32 z_index = 9.f) {
	imm_begin();
	imm_char(c, font, x, y, color, z_index);
	imm_flush();
}

ch::Vector2 imm_string(const ch::String& s, const Font& font, f32 x, f32 y, const ch::Color& color, f32 z_index = 9.f);
CH_FORCEINLINE ch::Vector2 draw_string(const ch::String& s, const Font& font, f32 x, f32 y, const ch::Color& color, f32 z_index = 9.f) {
	font.bind();
	imm_begin();
	const ch::Vector2 result = imm_string(s, font, x, y, color, z_index);
	imm_flush();
	return result;
}

CH_FORCEINLINE ch::Vector2 imm_string(const char* s, const Font& font, f32 x, f32 y, const ch::Color& color, f32 z_index = 9.f) {
	ch::String new_s;
	new_s.data = (char*)s; // I'm psure this is undefined behavior and bad but oh well
	new_s.count = ch::strlen(s);
	return imm_string(new_s, font, x, y, color, z_index);
}
CH_FORCEINLINE ch::Vector2 draw_string(const char* s, const Font& font, f32 x, f32 y, const ch::Color& color, f32 z_index = 9.f) {
	font.bind();
	imm_begin();
	const ch::Vector2 result = imm_string(s, font, x, y, color, z_index);
	imm_flush();
	return result;
}

ch::Vector2 get_string_draw_size(const ch::String& s, const Font& font);
CH_FORCEINLINE ch::Vector2 get_string_draw_size(const char* s, const Font& font) {
	ch::String new_s;
	new_s.data = (char*)s; // I'm psure this is undefined behavior and bad but oh well
	new_s.count = ch::strlen(s);
	return get_string_draw_size(new_s, font);
}

void imm_border_quad(f32 x0, f32 y0, f32 x1, f32 y1, f32 thickness, const ch::Color& color, f32 z_index = 9.f);
CH_FORCEINLINE void draw_border_quad(f32 x0, f32 y0, f32 x1, f32 y1, f32 thickness, const ch::Color& color, f32 z_index = 9.f) {
	imm_begin();
	imm_border_quad(x0, y0, x1, y1, thickness, color, z_index);
	imm_flush();
}
