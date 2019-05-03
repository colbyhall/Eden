#include "draw.h"

#include "os.h"
#include "parsing.h"
#include "editor.h"
#include "string.h"
#include "buffer.h"
#include "memory.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define MAX_VERTICES 3 * 1024

struct Vertex {
	Vector2 position;
	Color color;
	Vector2 uv;
};

GLuint imm_vao;
GLuint imm_vbo;
Vertex imm_vertices[MAX_VERTICES];
u32 imm_vertex_count;
Matrix4 view_to_projection;
Matrix4 world_to_view;

Shader solid_shape_shader;
Shader font_shader;

u32 draw_calls;
size_t verts_drawn;
size_t verts_culled;

Shader* current_shader = nullptr;

enum Shader_Type {
	ST_NONE,
	ST_FRAG,
	ST_VERT,
};

void shader_bind(Shader* shader) {	
	glUseProgram(shader->program_id);
	current_shader = shader;
}

static 
void parse_shader_source(String* fd, Shader_Type type, String* out_source) {
	if (type == ST_NONE) return;

	Shader_Type current_type = ST_NONE;
	String line;
	do {
		line = string_eat_line(fd);

		// @NOTE(Colby): this is super basic but it works.
		if (string_starts_with(line, "#if VERT", false) || string_starts_with(line, "#elif VERT", false)) {
			current_type = ST_VERT;
		}
		else if (string_starts_with(line, "#if FRAG", false) || string_starts_with(line, "#elif FRAG", false)) {
			current_type = ST_FRAG;
		}
		// @BUG(Colby): This needs to be true for some reason
		else if (string_starts_with(line, "#endif", true)) {
			current_type = ST_NONE;
		}
		else {
			if (current_type == type || current_type == ST_NONE) {
				string_append(out_source, line);
				string_append(out_source, "\n");
			}
		}

	} while (line.count > 0);

	out_source->data[out_source->count] = 0;
}

Shader shader_load_from_file(const char* path) {
	Shader result;
	GLuint program_id = glCreateProgram();

	GLuint vertex_id = glCreateShader(GL_VERTEX_SHADER);
	GLuint frag_id = glCreateShader(GL_FRAGMENT_SHADER);

	String shader_source = os_load_file_into_memory(path);

	u8* buffer = (u8*)c_alloc(shader_source.count * 2 + 2);

	String vert_source;
	vert_source.count = 0;
	vert_source.allocated = shader_source.count + 1;
	vert_source.data = buffer;

	String frag_source;
	frag_source.count = 0;
	frag_source.allocated = shader_source.count + 1;
	frag_source.data = buffer + shader_source.count + 1;

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

	c_free(buffer);
	c_free(shader_source.data);

	return result;
}

void draw_init() {
    glGenVertexArrays(1, &imm_vao);
    glBindVertexArray(imm_vao);
    
    glGenBuffers(1, &imm_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, imm_vbo);
    
    glBindVertexArray(0);

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
    
    glVertexAttribPointer(position_loc, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
    glEnableVertexAttribArray(position_loc);

    glVertexAttribPointer(color_loc, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)sizeof(Vector2));
    glEnableVertexAttribArray(color_loc);
    
    glVertexAttribPointer(uv_loc, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(sizeof(Vector2) + sizeof(Color)));
    glEnableVertexAttribArray(uv_loc);
    
    glDrawArrays(GL_TRIANGLES, 0, imm_vertex_count);
	draw_calls += 1;
	verts_drawn += imm_vertex_count;
    
    glDisableVertexAttribArray(position_loc);
    glDisableVertexAttribArray(color_loc);
    glDisableVertexAttribArray(uv_loc);
    
    glBindVertexArray(0);

}

static
Vertex* get_next_vertex_ptr() {
    return (Vertex*)&imm_vertices + imm_vertex_count;
}

void immediate_vertex(float x, float y, const Color& color, Vector2 uv) {
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

void immediate_quad(float x0, float y0, float x1, float y1, const Color& color) {
    immediate_vertex(x0, y0, color, v2(0.f, 0.f));
    immediate_vertex(x0, y1, color, v2(0.f, 1.f));
    immediate_vertex(x1, y0, color, v2(1.f, 0.f));
    
    immediate_vertex(x0, y1, color, v2(0.f, 1.f));
    immediate_vertex(x1, y1, color, v2(1.f, 1.f));
    immediate_vertex(x1, y0, color, v2(1.f, 0.f));
}


void draw_rect(float x0, float y0, float x1, float y1, const Color& color) {
	shader_bind(&solid_shape_shader);

    refresh_transformation();
    
    immediate_begin();
    immediate_quad(x0, y0, x1, y1, color);
    immediate_flush();
}

Vector2 immediate_string(const String& str, float x, float y, const Color& color) {
	const float font_height = FONT_SIZE;

	const float original_x = x;
	const float original_y = y;

	float largest_x = 0.f;
	float largest_y = 0.f;

	for (size_t i = 0; i < str.count; i++) {
		if (is_eol(str.data[i])) {
			y += font_height;
			x = original_x;
			verts_culled += 6;
			continue;
		}

		if (str.data[i] == '\t') {
			Font_Glyph space_glyph = font_find_glyph(&font, ' ');
			x += space_glyph.advance  * 4.f;
			verts_culled += 6 * 4;
			continue;
		}

		Font_Glyph& glyph = font_find_glyph(&font, str.data[i]);

		immediate_glyph(glyph, x, y, color);

		x += glyph.advance ;

		if (x - original_x > largest_x) largest_x = x - original_x;
		if (y - original_y > largest_y) largest_y = x - original_y;
	}

	return v2(largest_x, largest_y);
}

void immediate_glyph(const Font_Glyph& glyph, float x, float y, const Color& color) {
	const Color v4_color = color;

	const float x0 = x + glyph.bearing_x;
	const float y0 = y + glyph.bearing_y;
	const float x1 = x0 + glyph.width;
	const float y1 = y0 + glyph.height;

	if (x1 < 0.f || x0 > os_window_width() || y1 < 0.f || y0 > os_window_height()) {
		verts_culled += 6;
		return;
	}

	Vector2 bottom_right = v2(glyph.x1 / (float)FONT_ATLAS_DIMENSION, glyph.y1 / (float)FONT_ATLAS_DIMENSION);
	Vector2 bottom_left = v2(glyph.x1 / (float)FONT_ATLAS_DIMENSION, glyph.y0 / (float)FONT_ATLAS_DIMENSION);
	Vector2 top_right = v2(glyph.x0 / (float)FONT_ATLAS_DIMENSION, glyph.y1 / (float)FONT_ATLAS_DIMENSION);
	Vector2 top_left = v2(glyph.x0 / (float)FONT_ATLAS_DIMENSION, glyph.y0 / (float)FONT_ATLAS_DIMENSION);

	immediate_vertex(x0, y0, v4_color, top_left);
	immediate_vertex(x0, y1, v4_color, top_right);
	immediate_vertex(x1, y0, v4_color, bottom_left);

	immediate_vertex(x0, y1, v4_color, top_right);
	immediate_vertex(x1, y1, v4_color, bottom_right);
	immediate_vertex(x1, y0, v4_color, bottom_left);
}

Font_Glyph immediate_char(u8 c, float x, float y, const Color& color) {
	Font_Glyph glyph = font_find_glyph(&font, c);

	immediate_glyph(glyph, x, y, color);

	return glyph;
}

void draw_string(const String& str, float x, float y, const Color& color) {
	font_bind(&font);
	y += font.ascent;

	immediate_begin();
	immediate_string(str, x, y, color);
	immediate_flush();
}

Vector2 get_draw_string_size(const String& str) {
	const float font_height = FONT_SIZE;

	float y = 0.f;
	float x = 0.f;

	float largest_x = x;
	float largest_y = y;
	const float original_x = x;
	y += font.ascent;

	for (size_t i = 0; i < str.count; i++) {
		Font_Glyph glyph = font_find_glyph(&font, str[i]);

		if (str.data[i] == '\n') {
			y += font_height;
			x = original_x;
		} else if (str.data[i] == '\t') {
			Font_Glyph space_glyph = font_find_glyph(&font, ' ');
			x += space_glyph.advance  * 4.f;
		} else {
			float x0 = x + glyph.bearing_x ;
			float y0 = y + glyph.bearing_y ;
			float x1 = x0 + glyph.width ;
			float y1 = y0 + glyph.height ;
			x += glyph.advance ;
			if (y1 > largest_y) largest_y = y1;
		}


		if (x > largest_x) largest_x = x;
	}

	return v2(largest_x, largest_y);
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
    world_to_view      = m4_translate(v2(-width / 2.f, ortho_size));
    
    refresh_transformation();
}

void render_frame_begin() {
	draw_calls = 0;
	verts_drawn = 0;
	verts_culled = 0;
	glClear(GL_COLOR_BUFFER_BIT);
}

void render_frame_end() {

#if BUILD_DEBUG
	char buffer[256];
	sprintf_s(
		buffer, 256, "Draw Calls: %i\nVerts Drawn: %llu\nVerts Culled: %llu\nFPS: %i\nAllocations: %llu\nAllocated: %f KB",
		draw_calls, verts_drawn, verts_culled, fps, memory_num_allocations(), memory_amount_allocated() / 1024.f
	);
	Vector2 string_size = get_draw_string_size(buffer);
	Vector2 padding = v2(10.f);
	float x = os_window_width() - (string_size.x + padding.x);
	float y = 0.f;

	{

		float x0 = x;
		float y0 = y;
		float x1 = x0 + string_size.x + padding.x;
		float y1 = y0 + string_size.y + padding.y;

		draw_rect(x0, y0, x1, y1, 0xAAAAAA);
	}

	{
		float x0 = x + padding.x / 2.f;
		float y0 = y + padding.y / 2.f;
		draw_string(buffer, x0, y0, 0xFFFFFF);
	}
#endif

	gl_swap_buffers();
}

void draw_buffer_view(const Buffer_View& buffer_view, float x0, float y0, float x1, float y1) {
	Buffer* buffer = editor_find_buffer(buffer_view.buffer_id);
	if (!buffer) {
		return;
	}

	float x = x0;
	float y = y0;

	const float starting_x = x;
	const float starting_y = y;

	y += font.ascent - buffer_view.current_scroll_y;

	const float font_height = FONT_SIZE;
	const size_t buffer_count = buffer_get_count(*buffer);
	const size_t cursor_index = buffer_get_cursor_index(*buffer);
	const Font_Glyph space_glyph = font_find_glyph(&font, ' ');

	const size_t lines_scrolled = (size_t)(buffer_view.current_scroll_y / font_height);
	const size_t starting_index = buffer_get_line_index(*buffer, lines_scrolled);

	y += lines_scrolled * font_height;

	font_bind(&font);
	immediate_begin();

	size_t line_index = lines_scrolled;
	const char* format = "%llu: LS: %llu |";
	char out_line_size[20];
	sprintf_s(out_line_size, 20, format, line_index, buffer->eol_table[line_index]);

	x += immediate_string(out_line_size, x, y, 0xFFFF00).x;

	for (size_t i = starting_index; i < buffer_count; i++) {
		Color color = 0xd6b58d;
		const u32 c = (*buffer)[i];

		if (x > x1) {
			x = starting_x;
			y += font_height;
		}

		if (i == cursor_index) {
			Font_Glyph glyph = font_find_glyph(&font, c);

			if (is_whitespace(c)) {
				glyph = space_glyph;
			}

			immediate_flush();
			const float cursor_x0 = x;
			const float cursor_y0 = y - font.ascent;
			const float cursor_x1 = cursor_x0 + glyph.advance;
			const float cursor_y1 = y - font.descent;
			draw_rect(cursor_x0, cursor_y0, cursor_x1, cursor_y1, 0x81E38E);
			color = 0x052329;
			font_bind(&font);
			immediate_begin();
		}

		switch (c) {
		case ' ':
			x += space_glyph.advance;
			continue;
		case '\t':
			x += space_glyph.advance * 4.f;
			continue;
		case '\n':
			x = starting_x;
			y += font_height;

			line_index += 1;
			char out_line_size[20];
			sprintf_s(out_line_size, 20, format, line_index, buffer->eol_table[line_index]);

			x += immediate_string(out_line_size, x, y, 0xFFFF00).x;

			continue;
		default:
			Font_Glyph glyph = immediate_char((u8)c, x, y, color);
			x += glyph.advance;
		}

		if (y - font.ascent > y1) {
			verts_culled += (buffer_count - i) * 6;
			break;
		}
	}
	immediate_flush();

	if (cursor_index == buffer_count) {
		const float cursor_x0 = x;
		const float cursor_y0 = y - font.ascent;
		const float cursor_x1 = x0 + space_glyph.advance;
		const float cursor_y1 = y - font.descent;
		draw_rect(cursor_x0, cursor_y0, cursor_x1, cursor_y1, 0x81E38E);
	}

	// @NOTE(Colby): Drawing info bar here
	{
		const Vector2 padding = v2(2.f);
		const float bar_height = FONT_SIZE + padding.y;
		{
			const float info_bar_x0 = x0;
			const float info_bar_y0 = y1 - bar_height;
			const float info_bar_x1 = info_bar_x0 + x1;
			const float info_bar_y1 = info_bar_y0 + bar_height;
			draw_rect(info_bar_x0, info_bar_y0, info_bar_x1, info_bar_y1, 0xd6b58d);

			const float x = 10.f;
			const float y = info_bar_y0 + (padding.y / 2.f);
			char output_string[1024];
			sprintf_s(output_string, 1024, "%s      LN: %llu     COL: %llu", buffer->title.data, buffer->current_line_number, buffer->current_column_number);
			draw_string(output_string, x, y, 0x052329);
		}
	}
}