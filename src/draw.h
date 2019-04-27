#pragma once

#include "types.h"
#include "math.h"
#include "opengl.h"
#include "font.h"
#include "string.h"
#include "buffer.h"

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
 
extern Shader solid_shape_shader;
extern Shader font_shader;

void init_renderer();

void immediate_begin();
void immediate_flush();

void immediate_vertex(float x, float y, Vector4 color, Vector2 uv);

void render_frame_begin();
void render_frame_end();

void draw_rect(float x0, float y0, float x1, float y1, Vector4 color);
void draw_string(const String& str, float x, float y, float font_height, int color);

Vector2 immediate_string(const String& str, float x, float y, float font_height, int color);

Vector2 get_draw_string_size(String* str, float font_height, Font* font);

void refresh_transformation();
void render_right_handed();
