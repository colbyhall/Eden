#include "draw.h"
#include "editor.h"
#include "gui.h"
#include "config.h"

#define STB_RECT_PACK_IMPLEMENTATION
#include <stb/stb_rect_pack.h>
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb/stb_truetype.h>

struct Bitmap {
	s32 width, height;
	u8* data;
};

Font* bound_font;

bool load_font_from_path(const ch::Path& path, Font* out_font) {
	ch::File_Data fd;
	if (!ch::load_file_into_memory(path, &fd)) return false;

	Font font = {};
	stbtt_InitFont(&font.info, (const u8*)fd.data, stbtt_GetFontOffsetForIndex((const u8*)fd.data, 0));

	font.num_glyphs = font.info.numGlyphs;// @Temporary: info is opaque, but we are peeking :)

	font.codepoints = ch_new int[font.num_glyphs];
	ch::mem_zero(font.codepoints, font.num_glyphs * sizeof(s32));
	{
		u32 glyphs_found = 0;
		// Ask STBTT for the glyph indices.
		// @Temporary: linearly search the codepoint space because STBTT doesn't expose CP->glyph idx;
		//             later we will parse the ttf file in a similar way to STBTT.
		//             Linear search is exactly 17 times slower than parsing for 65536 glyphs.
		for (s32 codepoint = 0; codepoint < 0x110000; codepoint++) {
			const s32 idx = stbtt_FindGlyphIndex(&font.info, codepoint);
			if (idx <= 0) continue;
			glyphs_found += 1;
			font.codepoints[idx] = codepoint;
		}
	}

	f64 atlas_area = 0;
	{
		s32 x0 = 0;
		s32 x1 = 0;
		s32 y0 = 0;
		s32 y1 = 0;


		for (u32 i = 0; (u32)i < font.num_glyphs; i++) {
			stbtt_GetGlyphBox(&font.info, i, &x0, &y0, &x1, &y1);

			f64 w = (x1 - x0);
			f64 h = (y1 - y0);

			atlas_area += w * h;
		}
	}
	font.atlas_area = atlas_area;

	glGenTextures(ARRAYSIZE(font.atlas_ids), font.atlas_ids);
	
	*out_font = font;

	return true;
}

const Font_Glyph* Font::operator[](u32 c) const {
	const s32 idx = stbtt_FindGlyphIndex(&info, c);
	if (idx > 0) {
		assert((u32)idx < num_glyphs);
		return &atlases[size].glyphs[idx];
	}

	return nullptr;
}

void Font::pack_atlas() {
	if (size < 2) size = 2;
	if (size > 128) size = 128;

	s32 _ascent, _descent, _line_gap;
	stbtt_GetFontVMetrics(&info, &_ascent, &_descent, &_line_gap);

	const f32 font_scale = stbtt_ScaleForPixelHeight(&info, size);
	ascent = (f32)_ascent * font_scale;
	descent = (f32)_descent * font_scale;
	line_gap = (f32)_line_gap * font_scale;

	if (!atlases[size].w) {

		u32 h_oversample = 1; // @NOTE(Phillip): On a low DPI display this looks almost as good to me.
		u32 v_oversample = 1; //                 The jump from 1x to 4x is way bigger than 4x to 64x.

		if (size <= 36) {
			h_oversample = 2;
			v_oversample = 2;
		}
		if (size <= 12) {
			h_oversample = 4;
			v_oversample = 4;
		}
		if (size <= 8) {
			h_oversample = 8;
			v_oversample = 8;
		}

		Bitmap atlas;

		if (size <= 12) {
			atlas.width = 512 * h_oversample;
			atlas.height = 512 * v_oversample;
		}
		else {
			f64 area = atlas_area * h_oversample * v_oversample;
			area *= 1.0 + 1 / (ch::sqrt(size)); // fudge factor for small sizes

			area *= font_scale * font_scale;

			const f32 root = ch::sqrt((f32)area);

			u32 atlas_dimension = (u32)root;
			atlas_dimension = (atlas_dimension + 127) & ~127;

			atlas.width = atlas_dimension;
			atlas.height = atlas_dimension;
		}
		atlases[size].w = atlas.width;
		atlases[size].h = atlas.height;

		atlas.data = ch_new u8[atlas.width * atlas.height];
		defer(ch_delete[] atlas.data);

		stbtt_pack_context pc;
		stbtt_packedchar* pdata = ch_new stbtt_packedchar[num_glyphs];
		defer(ch_delete[] pdata);
		stbtt_pack_range pr;

		stbtt_PackBegin(&pc, atlas.data, atlas.width, atlas.height, 0, 1, NULL);
		pr.chardata_for_range = pdata;
		pr.array_of_unicode_codepoints = codepoints;
		pr.first_unicode_codepoint_in_range = 0;
		pr.num_chars = num_glyphs;
		pr.font_size = size;

		stbtt_PackSetSkipMissingCodepoints(&pc, 1);
		stbtt_PackSetOversampling(&pc, h_oversample, v_oversample);
		stbtt_PackFontRanges(&pc, info.data, 0, &pr, 1);
		stbtt_PackEnd(&pc);

		glBindTexture(GL_TEXTURE_2D, atlas_ids[size]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, atlas.width, atlas.height, 0, GL_RED, GL_UNSIGNED_BYTE, atlas.data);

		atlases[size].glyphs = ch_new Font_Glyph[num_glyphs];
		for (u32 i = 0; i < num_glyphs; i++) {
			Font_Glyph* glyph = &atlases[size].glyphs[i];

			glyph->x0 = pdata[i].x0;
			glyph->x1 = pdata[i].x1;
			glyph->y0 = pdata[i].y0;
			glyph->y1 = pdata[i].y1;

			glyph->width = ((f32)pdata[i].x1 - pdata[i].x0) / (f32)h_oversample;
			glyph->height = ((f32)pdata[i].y1 - pdata[i].y0) / (f32)v_oversample;
			glyph->bearing_x = pdata[i].xoff;
			glyph->bearing_y = pdata[i].yoff;
			glyph->advance = pdata[i].xadvance;
		}

		glBindTexture(GL_TEXTURE_2D, 0);
	}
}

struct Vertex {
	ch::Vector2 position;
	ch::Color color;
	ch::Vector2 uv;
	f32 z_index;
};

struct Shader {
	GLuint program_id;

	GLint projection_loc;
	GLint view_loc;

	GLint position_loc;
	GLint color_loc;
	GLint uv_loc;
	GLint z_index_loc;

	GLuint texture_loc;
};

#define MAX_VERTICES (3 * 1024)

GLuint imm_vao;
GLuint imm_vbo;
Vertex imm_vertices[MAX_VERTICES];
u32 imm_vertex_count;
ch::Matrix4 projection_matrix;
ch::Matrix4 view_matrix;

Shader global_shader;

const GLchar* global_shader_source = R"foo(
#ifdef VERTEX
layout(location = 0) in vec2 position;
layout(location = 1) in vec4 color;
layout(location = 2) in vec2 uv;
layout(location = 3) in float z_index;
uniform mat4 projection;
uniform mat4 view;
out vec4 out_color;
out vec2 out_uv;
void main() {
    gl_Position = projection * view * vec4(position, -z_index, 1.0);
	out_color = color;
	out_uv = uv;
}

#endif
#ifdef FRAGMENT
out vec4 frag_color;
in vec4 out_color;
in vec2 out_uv;
uniform sampler2D ftex;
void main() {
	if (out_uv.x < 0 && out_uv.y < 0) frag_color = out_color;
	else {
		vec4 sample = texture(ftex, out_uv);
		frag_color = vec4(out_color.xyz, sample.r);
	}
}
#endif
)foo";

static bool load_shader_from_source(const GLchar* source, Shader* out_shader) {
	Shader result;
	GLuint program_id = glCreateProgram();

	GLuint vertex_id = glCreateShader(GL_VERTEX_SHADER);
	GLuint frag_id = glCreateShader(GL_FRAGMENT_SHADER);

	const GLchar* shader_header = "#version 330 core\n#extension GL_ARB_separate_shader_objects: enable\n";

	const GLchar* vert_shader[3] = { shader_header, "#define VERTEX 1\n", source };
	const GLchar* frag_shader[3] = { shader_header, "#define FRAGMENT 1\n", source };

	glShaderSource(vertex_id, 3, vert_shader, 0);
	glShaderSource(frag_id, 3, frag_shader, 0);

	glCompileShader(vertex_id);
	glCompileShader(frag_id);

	glAttachShader(program_id, vertex_id);
	glAttachShader(program_id, frag_id);

	glLinkProgram(program_id);

	glValidateProgram(program_id);

	GLint is_linked = false;
	glGetProgramiv(program_id, GL_LINK_STATUS, &is_linked);
	if (!is_linked) {
		GLsizei ignored;
		char vert_errors[4096];
		char frag_errors[4096];
		char program_errors[4096];

		glGetShaderInfoLog(vertex_id, sizeof(vert_errors), &ignored, vert_errors);
		glGetShaderInfoLog(frag_id, sizeof(frag_errors), &ignored, frag_errors);
		glGetProgramInfoLog(program_id, sizeof(program_errors), &ignored, program_errors);
		return false;
	}

	glDeleteShader(vertex_id);
	glDeleteShader(frag_id);

	result.program_id = program_id;
	result.projection_loc = glGetUniformLocation(program_id, "projection");
	result.view_loc = glGetUniformLocation(program_id, "view");
	result.texture_loc = glGetUniformLocation(program_id, "ftex");
	result.position_loc = 0;
	result.color_loc = 1;
	result.uv_loc = 2;
	result.z_index_loc = 3;

	*out_shader = result;

	return true;
}

static void gl_error_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
	if (severity != GL_DEBUG_SEVERITY_NOTIFICATION) {
		ch::std_out << "GL CALLBACK: " << message << ch::eol;
	}
}

void init_draw() {
	assert(ch::is_gl_loaded());

	glGenVertexArrays(1, &imm_vao);
	glBindVertexArray(imm_vao);

	glGenBuffers(1, &imm_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, imm_vbo);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glEnable(GL_MULTISAMPLE);
	glEnable(GL_DEPTH_TEST);
	glClearDepth(1.f);
	glDepthFunc(GL_LEQUAL);
	glClearColor(ch::black);

#if BUILD_DEBUG
	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(gl_error_callback, 0);
#endif

#if CH_PLATFORM_WINDOWS
	wglSwapIntervalEXT(false);
#endif

	const bool global_shader_loaded = load_shader_from_source(global_shader_source, &global_shader);
	assert(global_shader_loaded);
	glUseProgram(global_shader.program_id);
}

static void frame_begin() {
	const ch::Vector2 viewport_size = the_window.get_viewport_size();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, viewport_size.ux, viewport_size.uy);
	render_right_handed();
}

static void frame_end() {
	ch::swap_buffers(the_window);
}

void draw_editor() {
	frame_begin();

	draw_gui();

	frame_end();
}

void refresh_shader_transform() {
	glUniformMatrix4fv(global_shader.view_loc, 1, GL_FALSE, view_matrix.elems);
	glUniformMatrix4fv(global_shader.projection_loc, 1, GL_FALSE, projection_matrix.elems);
}

void render_right_handed() {
	const ch::Vector2 viewport_size = the_window.get_viewport_size();

	const f32 width = (f32)viewport_size.ux;
	const f32 height = (f32)viewport_size.uy;

	const f32 aspect_ratio = width / height;

	const f32 f = 10.f;
	const f32 n = 1.f;

	const f32 ortho_size = height / 2.f;

	projection_matrix = ch::ortho(ortho_size, aspect_ratio, f, n);
	view_matrix       = ch::translate(ch::Vector2(-width / 2.f, ortho_size));

	refresh_shader_transform();
}

void immediate_begin() {
	imm_vertex_count = 0;
}

void immediate_flush() {
	if (imm_vertex_count == 0) return;

	glBindVertexArray(imm_vao);
	glBindBuffer(GL_ARRAY_BUFFER, imm_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(imm_vertices[0]) * imm_vertex_count, imm_vertices, GL_STREAM_DRAW);

	const GLuint position_loc = global_shader.position_loc;
	const GLuint color_loc = global_shader.color_loc;
	const GLuint uv_loc = global_shader.uv_loc;
	const GLuint z_index_loc = global_shader.z_index_loc;

	glVertexAttribPointer(position_loc, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
	glEnableVertexAttribArray(position_loc);

	glVertexAttribPointer(color_loc, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)sizeof(ch::Vector2));
	glEnableVertexAttribArray(color_loc);

	glVertexAttribPointer(uv_loc, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(sizeof(ch::Vector2) + sizeof(ch::Vector4)));
	glEnableVertexAttribArray(uv_loc);

	glVertexAttribPointer(z_index_loc, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(sizeof(ch::Vector2) + sizeof(ch::Vector4) + sizeof(ch::Vector2)));
	glEnableVertexAttribArray(z_index_loc);

	glDrawArrays(GL_TRIANGLES, 0, imm_vertex_count);

	glDisableVertexAttribArray(position_loc);
	glDisableVertexAttribArray(color_loc);
	glDisableVertexAttribArray(uv_loc);
	glDisableVertexAttribArray(z_index_loc);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

static Vertex* get_next_vertex_ptr() {
	return (Vertex*)&imm_vertices + imm_vertex_count;
}

void immediate_vertex(f32 x, f32 y, const ch::Color& color, ch::Vector2 uv, f32 z_index) {
	if (imm_vertex_count >= MAX_VERTICES) {
		immediate_flush();
		immediate_begin();
	}

	Vertex* vertex = get_next_vertex_ptr();

	vertex->position.x = x;
	vertex->position.y = -y;
	vertex->color = color;
	vertex->uv = uv;
	vertex->z_index = z_index;

    extern int num_vertices_total;
    num_vertices_total += 1;
	imm_vertex_count += 1;
}

void immediate_quad(f32 x0, f32 y0, f32 x1, f32 y1, const ch::Color& color, f32 z_index) {
	immediate_vertex(x0, y0, color, ch::Vector2(-1.f, -1.f), z_index);
	immediate_vertex(x0, y1, color, ch::Vector2(-1.f, -1.f), z_index);
	immediate_vertex(x1, y0, color, ch::Vector2(-1.f, -1.f), z_index);

	immediate_vertex(x0, y1, color, ch::Vector2(-1.f, -1.f), z_index);
	immediate_vertex(x1, y1, color, ch::Vector2(-1.f, -1.f), z_index);
	immediate_vertex(x1, y0, color, ch::Vector2(-1.f, -1.f), z_index);
}

void Font::bind() const {
	refresh_shader_transform();
	glUniform1i(global_shader.texture_loc, 0);

	glBindTexture(GL_TEXTURE_2D, atlas_ids[size]);
	glActiveTexture(GL_TEXTURE0);
}


void immediate_glyph(const Font_Glyph& glyph, const Font& font, f32 x, f32 y, const ch::Color& color, f32 z_index /*= 9.f*/) {
	// @NOTE(CHall): draw glyphs top down
	y += the_font.size;
	
	const f32 x0 = x + glyph.bearing_x;
	const f32 y0 = y + glyph.bearing_y;
	const f32 x1 = x0 + glyph.width;
	const f32 y1 = y0 + glyph.height;

	const f32 atlas_w = (f32)font.atlases[font.size].w;
	const f32 atlas_h = (f32)font.atlases[font.size].h;

	const ch::Vector2 bottom_right = ch::Vector2(glyph.x1 / atlas_w, glyph.y1 / atlas_h);
	const ch::Vector2 bottom_left = ch::Vector2(glyph.x1 / atlas_w, glyph.y0 / atlas_h);
	const ch::Vector2 top_right = ch::Vector2(glyph.x0 / atlas_w, glyph.y1 / atlas_h);
	const ch::Vector2 top_left = ch::Vector2(glyph.x0 / atlas_w, glyph.y0 / atlas_h);

	immediate_vertex(x0, y0, color, top_left, z_index);
	immediate_vertex(x0, y1, color, top_right, z_index);
	immediate_vertex(x1, y0, color, bottom_left, z_index);

	immediate_vertex(x0, y1, color, top_right, z_index);
	immediate_vertex(x1, y1, color, bottom_right, z_index);
	immediate_vertex(x1, y0, color, bottom_left, z_index);
}

const Font_Glyph* immediate_char(const u32 c, const Font& font, f32 x, f32 y, const ch::Color& color, f32 z_index /*= 9.f*/) {
	const Font_Glyph* g = font[c];
	if (!g) {
		g = font['?'];
		assert(g);
		immediate_glyph(*g, font, x, y, ch::magenta, z_index);
		return nullptr;
	}

	immediate_glyph(*g, font, x, y, color, z_index);
	return g;
}

ch::Vector2 immediate_string(const ch::String& s, const Font& font, f32 x, f32 y, const ch::Color& color, f32 z_index /*= 9.f*/) {
	const f32 font_height = font.size;

	const f32 original_x = x;
	const f32 original_y = y;

	f32 largest_x = 0.f;
	f32 largest_y = 0.f;

	for (usize i = 0; i < s.count; i++) {
		if (s[i] == ch::eol) {
			y += font_height;
			x = original_x;
			continue;
		}

		if (s[i] == '\t') {
			const Font_Glyph* space_glyph = font[' '];
			assert(space_glyph);
			x += space_glyph->advance  * get_config().tab_width;
			continue;
		}

		const Font_Glyph* glyph = font[s[i]];

		if (glyph) {
			immediate_glyph(*glyph, font, x, y, color, z_index);

			x += glyph->advance;
		}

		if (x - original_x > largest_x) largest_x = x - original_x;
		if (y - original_y > largest_y) largest_y = x - original_y;
	}

	return ch::Vector2(largest_x, largest_y);
}

ch::Vector2 get_string_draw_size(const ch::String& s, const Font& font) {
	const f32 font_height = font.size;

	const f32 starting_x = 0.f;
	const f32 starting_y = 0.f;

	f32 largest_x = 0.f;
	f32 largest_y = 0.f;

	f32 x = starting_x;
	f32 y = starting_y;

	const Font_Glyph* space_glyph = font[' '];
	const Font_Glyph* unknown_glyph = font['?'];

	for (usize i = 0; i < s.count; i++) {
		if (s[i] == ch::eol) {
			y += font_height;
			x = starting_x;
			continue;
		}

		if (s[i] == '\t') {
			x += space_glyph->advance  * get_config().tab_width;
			continue;
		}

		const Font_Glyph* g = font[s[i]];

		if (!g) {
			g = unknown_glyph;
		}

		x += g->advance;

		if (x - starting_x > largest_x) largest_x = x - starting_x;
		if (y - starting_y > largest_y) largest_y = x - starting_y;
	}

	return ch::Vector2(largest_x, largest_y + font_height);
}

void immediate_border_quad(f32 x0, f32 y0, f32 x1, f32 y1, f32 thickness, const ch::Color& color, f32 z_index /*= 9.f*/)
{
	{
		const f32 _x0 = x0;
		const f32 _y0 = y0;
		const f32 _x1 = _x0 + thickness;
		const f32 _y1 = y1;
		immediate_quad(_x0, _y0, _x1, _y1, color, z_index);
	}

	{
		const f32 _x0 = x1 - thickness;
		const f32 _y0 = y0;
		const f32 _x1 = _x0 + thickness;
		const f32 _y1 = y1;
		immediate_quad(_x0, _y0, _x1, _y1, color, z_index);
	}

	{
		const f32 _x0 = x0;
		const f32 _y0 = y0;
		const f32 _x1 = x1;
		const f32 _y1 = _y0 + thickness;
		immediate_quad(_x0, _y0, _x1, _y1, color, z_index);
	}

	{
		const f32 _x0 = x0;
		const f32 _y0 = y1 - thickness;
		const f32 _x1 = x1;
		const f32 _y1 = _y0 + thickness;
		immediate_quad(_x0, _y0, _x1, _y1, color, z_index);
	}
}
