#pragma once

#include "types.h"
#include "math.h"
#include "opengl.h"
#include "font.h"
#include "string.h"

typedef struct Vertex {
	Vector2 position;
	Vector4 color;
	Vector2 uv;
} Vertex;

typedef struct Shader {
	GLuint program_id;

	GLint view_to_projection_loc;
	GLint world_to_view_loc;

	GLint position_loc;
	GLint color_loc;
	GLint uv_loc;

	GLuint texture_loc;
} Shader;

void bind_shader(Shader* shader);

extern Matrix4 view_to_projection;
extern Matrix4 world_to_view;

extern Shader solid_shape_shader;
extern Shader font_shader;

void refresh_transformation();

void init_renderer();

void immediate_begin();
void immediate_flush();

void draw_rect(float x0, float y0, float x1, float y1, Vector4 color);
// void draw_font_atlas(float x0, float y0, float x1, float y1, Font* font);
void draw_string(String* str, float x, float y, float font_height, Font* font);
// void draw_framebuffer(const Framebuffer& fb, float x0, float y0, float x1, float y1);
// void draw_gap_buffer(Gap_Buffer* buffer, float x, float y, float width, float height, float font_height, Font* font);

void render_right_handed();
