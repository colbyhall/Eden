#include "draw.h"
#include "os.h"
#include "parsing.h"
#include "editor.h"

#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define MAX_VERTICES 30000

GLuint imm_vao;
GLuint imm_vbo;
Vertex imm_vertices[MAX_VERTICES];
u32 imm_vertex_count;
Matrix4 view_to_projection;
Matrix4 world_to_view;

Shader* current_shader;

Shader solid_shape_shader;
Shader font_shader;

u32 draw_calls;
u32 verts_drawn;
u32 verts_culled;

typedef enum Shader_Type {
	ST_NONE,
	ST_FRAG,
	ST_VERT,
} Shader_Type;

void bind_shader(Shader* shader) {	
	if (shader == NULL) {
		glUseProgram(0);
	} else {
		glUseProgram(shader->program_id);
	}
	current_shader = shader;
}

void parse_shader_source(String* fd, Shader_Type type, String* out_source) {
	if (type == ST_NONE) return;

	Shader_Type current_type = ST_NONE;
	String line;
	do {
		line = string_eat_line(fd);

		// @NOTE(Colby): this is super basic but it works.
		if (string_starts_with(&line, "#if VERT", false) || string_starts_with(&line, "#elif VERT", false)) {
			current_type = ST_VERT;
		}
		else if (string_starts_with(&line, "#if FRAG", false) || string_starts_with(&line, "#elif FRAG", false)) {
			current_type = ST_FRAG;
		}
		// @BUG(Colby): This needs to be true for some reason
		else if (string_starts_with(&line, "#endif", true)) {
			current_type = ST_NONE;
		}
		else {
			if (current_type == type || current_type == ST_NONE) {
				append_string(out_source, &line);
				append_cstring(out_source, "\n");
			}
		}

	} while (line.length > 0);

	out_source->data[out_source->length] = 0;
}

Shader shader_load_from_file(const char* path) {
	Shader result;
	GLuint program_id = glCreateProgram();

	GLuint vertex_id = glCreateShader(GL_VERTEX_SHADER);
	GLuint frag_id = glCreateShader(GL_FRAGMENT_SHADER);

	String shader_source = os_load_file_into_memory(path);
	// @NOTE(Colby): copy so we can free

	u8* buffer = (u8*)malloc(shader_source.length * 2 + 2);

	String vert_source;
	vert_source.length = 0;
	vert_source.allocated = shader_source.length + 1;
	vert_source.data = buffer;

	String frag_source;
	frag_source.length = 0;
	frag_source.allocated = shader_source.length + 1;
	frag_source.data = buffer + shader_source.length + 1;

	String fd = shader_source;
	parse_shader_source(&fd, ST_VERT, &vert_source);
	fd = shader_source;
	parse_shader_source(&fd, ST_FRAG, &frag_source);

	glShaderSource(vertex_id, 1, (const char**)&vert_source.data, 0);
	glShaderSource(frag_id, 1, (const char**)&frag_source.data, 0);

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

		printf("%s\n%s\n%s\n", vert_errors, frag_errors, program_errors);
		assert(!"Shader validation failed");
	}

	glDeleteShader(vertex_id);
	glDeleteShader(frag_id);

	result.program_id = program_id;

	result.view_to_projection_loc = glGetUniformLocation(program_id, "view_to_projection");
	result.world_to_view_loc = glGetUniformLocation(program_id, "world_to_view");
	result.texture_loc = glGetUniformLocation(program_id, "ftex");
	result.position_loc = 0;
	result.color_loc = 1;
	result.uv_loc = 2;

	// @Cleanup: Why can't we free this
	// p_delete[] buffer;
	// free_string(&shader_source);
	// free(buffer);

	return result;
}

void init_renderer() {
    glGenVertexArrays(1, &imm_vao);
    glBindVertexArray(imm_vao);
    
    glGenBuffers(1, &imm_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, imm_vbo);
    
    glBindVertexArray(0);

	imm_vertex_count = 0;
	view_to_projection = m4_identity();
	world_to_view = m4_identity();

	render_right_handed();

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glEnable(GL_MULTISAMPLE);

	solid_shape_shader = shader_load_from_file("data\\shaders\\solid_shape.glsl");
	font_shader = shader_load_from_file("data\\shaders\\font.glsl");
}

void immediate_begin() {
    imm_vertex_count = 0;
}

bool is_valid(GLuint id) {
    return id != -1;
}

void immediate_flush() {
    if (!current_shader)
    {
        assert(!"We should have a shader set");
        return;
    }
    
    glBindVertexArray(imm_vao);
    glBindBuffer(GL_ARRAY_BUFFER, imm_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(imm_vertices[0]) * imm_vertex_count, imm_vertices, GL_STREAM_DRAW);
    
    GLuint position_loc = current_shader->position_loc;
    GLuint color_loc = current_shader->color_loc;
    GLuint uv_loc = current_shader->uv_loc;
    
    if (is_valid(position_loc)) {
        glVertexAttribPointer(position_loc, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
        glEnableVertexAttribArray(position_loc);
    }
    
    if (is_valid(color_loc)) {
        glVertexAttribPointer(color_loc, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)sizeof(Vector2));
        glEnableVertexAttribArray(color_loc);
    }
    
    if (is_valid(uv_loc)) {
        glVertexAttribPointer(uv_loc, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(sizeof(Vector2) + sizeof(Vector4)));
        glEnableVertexAttribArray(uv_loc);
    }
    
    glDrawArrays(GL_TRIANGLES, 0, imm_vertex_count);
	draw_calls += 1;
	verts_drawn += imm_vertex_count;
    
    glDisableVertexAttribArray(position_loc);
    glDisableVertexAttribArray(color_loc);
    glDisableVertexAttribArray(uv_loc);
    
    glBindVertexArray(0);

}

Vertex* get_next_vertex_ptr() {
    return (Vertex*)&imm_vertices + imm_vertex_count;
}

void immediate_vertex(float x, float y, Vector4 color, Vector2 uv) {
	if (imm_vertex_count >= MAX_VERTICES) {
		immediate_flush();
		immediate_begin();
	}
    
    Vertex* vertex = get_next_vertex_ptr();
    
    vertex->position.x = x;
    vertex->position.y = -y;
    vertex->color      = color;
    vertex->uv         = uv;
    
    imm_vertex_count += 1;
}

void immediate_quad(float x0, float y0, float x1, float y1, Vector4 color) {
    immediate_vertex(x0, y0, color, vec2(0.f, 0.f));
    immediate_vertex(x0, y1, color, vec2(0.f, 1.f));
    immediate_vertex(x1, y0, color, vec2(1.f, 0.f));
    
    immediate_vertex(x0, y1, color, vec2(0.f, 1.f));
    immediate_vertex(x1, y1, color, vec2(1.f, 1.f));
    immediate_vertex(x1, y0, color, vec2(1.f, 0.f));
}


void draw_rect(float x0, float y0, float x1, float y1, Vector4 color) {   
	bind_shader(&solid_shape_shader);

    refresh_transformation();
    
    immediate_begin();
    immediate_quad(x0, y0, x1, y1, color);
    immediate_flush();
}

void immediate_string(String* str, float x, float y, float font_height, int color) {

	const float ratio = font_height / FONT_SIZE;

	const float original_x = x;
	y += font_height - font.line_gap * ratio;

	for (size_t i = 0; i < str->length; i++) {

		if (str->data[i] == '\n') {
			y += font_height;
			x = original_x;
			continue;
		}

		if (str->data[i] == '\t') {
			Font_Glyph space_glyph = font.characters[' ' - 32];
			x += space_glyph.advance * ratio * 4.f;
			continue;
		}

		Font_Glyph glyph = font.characters[str->data[i] - 32];

		if (!is_whitespace(str->data[i])) {
			Vector4 v4_color = vec4_color(color);

			float x0 = x + glyph.bearing_x * ratio;
			float y0 = y + glyph.bearing_y * ratio;
			float x1 = x0 + glyph.width * ratio;
			float y1 = y0 + glyph.height * ratio;

			if (x0 > os_window_width() || x1 < 0.f || y > os_window_height() || y + font_height < 0.f) {
				verts_culled += 6;
				continue;
			}

			Vector2 bottom_right = vec2(glyph.x1 / (float)FONT_ATLAS_DIMENSION, glyph.y1 / (float)FONT_ATLAS_DIMENSION);
			Vector2 bottom_left = vec2(glyph.x1 / (float)FONT_ATLAS_DIMENSION, glyph.y0 / (float)FONT_ATLAS_DIMENSION);
			Vector2 top_right = vec2(glyph.x0 / (float)FONT_ATLAS_DIMENSION, glyph.y1 / (float)FONT_ATLAS_DIMENSION);
			Vector2 top_left = vec2(glyph.x0 / (float)FONT_ATLAS_DIMENSION, glyph.y0 / (float)FONT_ATLAS_DIMENSION);

			immediate_vertex(x0, y0, v4_color, top_left);
			immediate_vertex(x0, y1, v4_color, top_right);
			immediate_vertex(x1, y0, v4_color, bottom_left);

			immediate_vertex(x0, y1, v4_color, top_right);
			immediate_vertex(x1, y1, v4_color, bottom_right);
			immediate_vertex(x1, y0, v4_color, bottom_left);
		}

		x += glyph.advance * ratio;
	}
}

void immediate_cstring(const char* str, float x, float y, float font_height, int color) {
	String cstr_s = make_string(str);
	immediate_string(&cstr_s, x, y, font_height, color);
}

void draw_string(String* str, float x, float y, float font_height, int color) {
	bind_shader(&font_shader);
	refresh_transformation();
	glUniform1i(font_shader.texture_loc, 0);

	glBindTexture(GL_TEXTURE_2D, font.texture_id);
	glActiveTexture(GL_TEXTURE0);

	immediate_begin();
	immediate_string(str, x, y, font_height, color);
	immediate_flush();
}

Vector2 get_draw_string_size(String* str, float font_height, Font* font) {
	const float ratio = font_height / FONT_SIZE;

	float y = 0.f;
	float x = 0.f;

	float largest_x = x;
	float largest_y = y;
	const float original_x = x;
	y += font_height - font->line_gap * ratio;

	for (size_t i = 0; i < str->length; i++) {
		Font_Glyph glyph = font->characters[str->data[i] - 32];

		if (str->data[i] == '\n') {
			y += font_height;
			x = original_x;
		} else if (str->data[i] == '\t') {
			Font_Glyph space_glyph = font->characters[' ' - 32];
			x += space_glyph.advance * ratio * 4.f;
		} else {
			float x0 = x + glyph.bearing_x * ratio;
			float y0 = y + glyph.bearing_y * ratio;
			float x1 = x0 + glyph.width * ratio;
			float y1 = y0 + glyph.height * ratio;
			x += glyph.advance * ratio;
			if (y1 > largest_y) largest_y = y1;
		}


		if (x > largest_x) largest_x = x;
	}

	return vec2(largest_x, largest_y);
}

void refresh_transformation() {
    if (!current_shader) return;
    
    glUniformMatrix4fv(current_shader->world_to_view_loc, 1, GL_FALSE, world_to_view.elems);
    glUniformMatrix4fv(current_shader->view_to_projection_loc, 1, GL_FALSE, view_to_projection.elems);
}

void render_right_handed() {
    const float width = (f32)os_window_width();
    const float height = (f32)os_window_height();
    
    const float aspect_ratio = width / height;
    
    const float f = 1.f;
    const float n = -1.f;
    
    const float ortho_size = height / 2.f;
    
    view_to_projection = m4_ortho(ortho_size, aspect_ratio, f, n);
    world_to_view      = m4_translate(vec2(-width / 2.f, ortho_size));
    
    refresh_transformation();
}

void render_frame_begin() {
	draw_calls = 0;
	verts_drawn = 0;
	verts_culled = 0;
	glClear(GL_COLOR_BUFFER_BIT);
}

void render_frame_end() {

	if (BUILD_DEBUG)
	{

		char buffer[256];
		sprintf_s((char* const)&buffer, 256, "Draw Calls: %i\nVerts Drawn: %i\nVerts Culled: %i\nFPS: %i", draw_calls, verts_drawn, verts_culled, fps);
		String debug_string;
		debug_string.data = (u8*)&buffer;
		debug_string.allocated = 256;
		debug_string.length = strlen(buffer);

		Vector2 string_size = get_draw_string_size(&debug_string, FONT_SIZE, &font);
		Vector2 padding = vec2s(10.f);
		float x = os_window_width() - (string_size.x + padding.x);
		float y = 0.f;

		{

			float x0 = x;
			float y0 = y;
			float x1 = x0 + string_size.x + padding.x;
			float y1 = y0 + string_size.y + padding.y;

			draw_rect(x0, y0, x1, y1, vec4_color(0xAAAAAA));
		}

		{
			float x0 = x + padding.x / 2.f;
			float y0 = y + padding.y / 2.f;
			draw_string(&debug_string, x0, y0, 20.f, 0xFFFFFF);
		}
	}

	gl_swap_buffers();
}