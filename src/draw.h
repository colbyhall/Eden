#pragma once

#include "types.h"
#include "opengl.h"
#include "math.h"
#include "font.h"

struct Buffer;
struct String;

struct Vertex {
	Vector2 position;
	Vector4 color;
	Vector2 uv;
};

struct Shader {
	GLuint program_id;

	GLint view_to_projection_loc;
	GLint world_to_view_loc;

	GLint position_loc;
	GLint color_loc;
	GLint uv_loc;

	GLuint texture_loc;

	void bind();

	static Shader load_from_file(const char* path);
	static Shader* current;
};

extern Matrix4 view_to_projection;
extern Matrix4 world_to_view;

extern size_t verts_culled;
 
extern Shader solid_shape_shader;
extern Shader font_shader;

void init_renderer();

void immediate_begin();
void immediate_flush();

void render_frame_begin();
void render_frame_end();

void draw_rect(float x0, float y0, float x1, float y1, const Vector4& color);
void draw_string(const String& str, float x, float y, const Vector4& color);

Vector2 immediate_string(const String& str, float x, float y, const Vector4& color);

void immediate_glyph(const Font_Glyph& glyph, float x, float y, const Vector4& color);
const Font_Glyph& immediate_char(u8 c, float x, float y, const Vector4& color);

Vector2 get_draw_string_size(const String& str);

void refresh_transformation();
void render_right_handed();
