#pragma once

#include "types.h"
#include "opengl.h"
#include "math.h"
#include "font.h"

struct Buffer;
struct Buffer_View;
struct String;

struct Shader {
	GLuint program_id;

	GLint view_to_projection_loc;
	GLint world_to_view_loc;

	GLint position_loc;
	GLint color_loc;
	GLint uv_loc;

	GLuint texture_loc;
};

void shader_bind(Shader* shader);
Shader shader_load_from_file(const char* path);

extern Matrix4 view_to_projection;
extern Matrix4 world_to_view;
 
extern Shader solid_shape_shader;
extern Shader font_shader;

void draw_init();

void immediate_begin();
void immediate_flush();

Vector2 immediate_string(const String& str, float x, float y, const Color& color);

void immediate_glyph(const Font_Glyph& glyph, float x, float y, const Color& color);
Font_Glyph immediate_char(u8 c, float x, float y, const Color& color);

void refresh_transformation();
void render_right_handed();

void draw_rect(float x0, float y0, float x1, float y1, const Color& color);
void draw_string(const String& str, float x, float y, const Color& color);
void draw_buffer_view(const Buffer_View& buffer_view, float x0, float y0, float x1, float y1);

void render_frame_begin();
void render_frame_end();

Vector2 get_draw_string_size(const String& str);
