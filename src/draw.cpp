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

struct Shader {
	GLuint program_id;

	GLint view_to_projection_loc;
	GLint world_to_view_loc;

	GLint position_loc;
	GLint color_loc;
	GLint uv_loc;

	GLuint texture_loc;
};

GLuint imm_vao;
GLuint imm_vbo;
Vertex imm_vertices[MAX_VERTICES];
u32 imm_vertex_count;
Matrix4 view_to_projection;
Matrix4 world_to_view;

Shader global_shader;

u32 draw_calls;
size_t verts_drawn;
size_t verts_culled;

Shader* current_shader = nullptr;

const GLchar* frag_shader = R"foo(
#version 330 core

out vec4 frag_color;

in vec4 out_color;
in vec2 out_uv;

uniform sampler2D ftex;

void main() {
	if (out_uv.x < 0 || out_uv.y < 0) {
		frag_color = out_color;
	} else {
		vec4 result = texture(ftex, out_uv);

		result.w = 1.0;

		frag_color = vec4(out_color.xyz, texture(ftex, out_uv).r);
	}
}
)foo";

const GLchar* vert_shader = R"foo(
#version 330 core

layout(location = 0) in vec2 position;
layout(location = 1) in vec4 color;
layout(location = 2) in vec2 uv;

uniform mat4 view_to_projection;
uniform mat4 world_to_view;

out vec4 out_color;
out vec2 out_uv;

void main() {
    gl_Position =  view_to_projection * world_to_view * vec4(position, 0.0, 1.0);
	out_color = color;
	out_uv = uv;
}
)foo";

Shader load_global_shader() {
	Shader result;
	GLuint program_id = glCreateProgram();

	GLuint vertex_id = glCreateShader(GL_VERTEX_SHADER);
	GLuint frag_id = glCreateShader(GL_FRAGMENT_SHADER);

	glShaderSource(vertex_id, 1, &vert_shader, 0);
	glShaderSource(frag_id, 1, &frag_shader, 0);

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

	return result;
}

void bind_font(Font* font) {
	refresh_transformation();
	glUniform1i(global_shader.texture_loc, 0);

	glBindTexture(GL_TEXTURE_2D, font->texture_id);
	glActiveTexture(GL_TEXTURE0);
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

	global_shader = load_global_shader();
	glUseProgram(global_shader.program_id);
	current_shader = &global_shader;
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
    immediate_vertex(x0, y0, color, v2(-1.f, -1.f));
    immediate_vertex(x0, y1, color, v2(-1.f, -1.f));
    immediate_vertex(x1, y0, color, v2(-1.f, -1.f));
    
    immediate_vertex(x0, y1, color, v2(-1.f, -1.f));
    immediate_vertex(x1, y1, color, v2(-1.f, -1.f));
    immediate_vertex(x1, y0, color, v2(-1.f, -1.f));
}


void draw_rect(float x0, float y0, float x1, float y1, const Color& color) {    
    immediate_begin();
    immediate_quad(x0, y0, x1, y1, color);
    immediate_flush();
}

Vector2 immediate_string(const String& str, float x, float y, const Color& color, const Font& font) {
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

		immediate_glyph(glyph, x, y, color, font);

		x += glyph.advance ;

		if (x - original_x > largest_x) largest_x = x - original_x;
		if (y - original_y > largest_y) largest_y = x - original_y;
	}

	return v2(largest_x, largest_y);
}

void immediate_glyph(const Font_Glyph& glyph, float x, float y, const Color& color, const Font& font) {
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

Font_Glyph immediate_char(u32 c, float x, float y, const Color& color, const Font& font) {
	Font_Glyph glyph = font_find_glyph(&font, c);

	immediate_glyph(glyph, x, y, color, font);

	return glyph;
}

void draw_string(const String& str, float x, float y, const Color& color, const Font& font) {
	y += font.ascent;

	immediate_begin();
	immediate_string(str, x, y, color, font);
	immediate_flush();
}

Vector2 get_draw_string_size(const String& str, const Font& font) {
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
	glViewport(0, 0, os_window_width(), os_window_height());
	render_right_handed();
}

void render_frame_end() {
	gl_swap_buffers();
}

static Color get_color(Syntax_Highlight_Type type) {
    switch (type) {
	case SHT_COMMENT: return 0x40c040;
    case SHT_IDENT: return 0xd6b58d;
    case SHT_KEYWORD: return 0xffffff;
    case SHT_OPERATOR: return 0xcccccc; // @Temporary
    case SHT_TYPE: return 0x66d9ef; // @Temporary
    case SHT_FUNCTION: return 0x77f777; // @Temporary
    //SHT_GENERIC_TYPE,
    //SHT_GENERIC_FUNCTION,
    case SHT_MACRO: return 0x7777f7; // @Temporary?
    case SHT_NUMERIC_LITERAL: return 0x80f0e0;
    case SHT_STRING_LITERAL: return 0x40b0a0;
    case SHT_DIRECTIVE: return 0xb0ffb0;
    case SHT_ANNOTATION: return 0xb0ffb0;
    default: assert(0);
    }
    return 0; // shut up compiler
}

void draw_buffer_view(Buffer_View* view, float x0, float y0, float x1, float y1, const Font& font) {
	float x = x0;
	float y = y0;

	const float starting_x = x;
	const float starting_y = y;

	Buffer* buffer = editor_find_buffer(&g_editor, view->id);
	assert(buffer);

    if (buffer->syntax_is_dirty) {
        parse_syntax(buffer);
    }

    y += font.ascent;

	const float font_height = FONT_SIZE;
#if GAP_BUFFER_DEBUG
	const size_t buffer_count = buffer->allocated;
#else
	const size_t buffer_count = get_count(*buffer);
#endif
	const Font_Glyph space_glyph = font_find_glyph(&font, ' ');

	const size_t lines_scrolled = (size_t)(view->current_scroll_y / font_height);
	const size_t starting_index = get_line_index(*buffer, lines_scrolled);

	if (buffer_count) y -= view->current_scroll_y;

	y += lines_scrolled * font_height;

    const Syntax_Highlight *current_highlight = buffer->syntax.data;
    
	immediate_begin();

	for (size_t i = starting_index; i < buffer_count; i++) {
#if LINE_COUNT_DEBUG
	size_t line_index = lines_scrolled;
	const char* format = "%llu: LS: %llu |";
	char out_line_size[20];
	sprintf_s(out_line_size, 20, format, line_index, buffer->eol_table[line_index]);

	x += immediate_string(out_line_size, x, y, 0xFFFF00, font).x;
#endif

#if GAP_BUFFER_DEBUG
		Color color = 0xd6b58d; //  get_color((Syntax_Highlight_Type)current_highlight->type);

		u32* current_point = buffer->data + i;
		u32 c = *current_point;

		if (current_point >= buffer->gap && current_point < buffer->gap + buffer->gap_size)
		{
			color = 0xFF00FF;
			c = '*';
			if (current_point == buffer->gap) {
				c = '[';
				color = 0xFFFF00;
			} else if (current_point == buffer->gap + buffer->gap_size - 1) {
				c = ']';
				color = 0xFFFF00;
			}

		}
#else
		const u32 c = (*buffer)[i];
        while (i >= current_highlight[1].where) {
            current_highlight += 1;
            assert(current_highlight + 1 < buffer->syntax.cend());
        }

        Color color = get_color((Syntax_Highlight_Type)current_highlight->type);
#endif

		if (x > x1) {
			x = starting_x;
			y += font_height;
		}

#if GAP_BUFFER_DEBUG
		if (current_point == get_index_as_cursor(buffer, view->cursor))
#else
		if (i == view->cursor)
#endif
        {
			Font_Glyph glyph = font_find_glyph(&font, c);

			if (is_whitespace(c)) {
				glyph = space_glyph;
			}

			const float cursor_x0 = x;
			const float cursor_y0 = y - font.ascent;
			const float cursor_x1 = cursor_x0 + glyph.advance;
			const float cursor_y1 = y - font.descent;
			immediate_quad(cursor_x0, cursor_y0, cursor_x1, cursor_y1, 0x81E38E);
			color = 0x052329;
		}

        if ((view->cursor > view->selection && i >= view->selection && i < view->cursor) || 
            (view->cursor < view->selection && i < view->selection && i > view->cursor)) {
            Font_Glyph glyph = font_find_glyph(&font, c);

            if (is_whitespace(c)) {
                glyph = space_glyph;
            }

            float width = glyph.advance;
            if (c == '\t') {
                width *= 4.f;
            }

            const float selection_x0 = x;
            const float selection_y0 = y - font.ascent;
            const float selection_x1 = selection_x0 + width;
            const float selection_y1 = y - font.descent;
            immediate_quad(selection_x0, selection_y0, selection_x1, selection_y1, 0x000EFF);
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

#if LINE_COUNT_DEBUG
			line_index += 1;

			if (buffer->eol_table.count > line_index)
			{
				char out_line_size[20];
				sprintf_s(out_line_size, 20, format, line_index, buffer->eol_table[line_index]);

				x += immediate_string(out_line_size, x, y, 0xFFFF00, font).x;
			}
#endif
			continue;
		default:
			Font_Glyph glyph = immediate_char(c, x, y, color, font);
			x += glyph.advance;
		}

		if (y - font.ascent > y1) {
			verts_culled += (buffer_count - i) * 6;
			break;
		}
	}

	// @NOTE(Colby): Drawing info bar here
	{
		const float font_height = FONT_SIZE;
		const Vector2 padding = v2(font_height / 2.f);
		const float bar_height = font_height + padding.y;
		{
			const float info_bar_x0 = x0;
			const float info_bar_y0 = y1 - bar_height;
			const float info_bar_x1 = info_bar_x0 + x1;
			const float info_bar_y1 = info_bar_y0 + bar_height;
			immediate_quad(info_bar_x0, info_bar_y0, info_bar_x1, info_bar_y1, 0xd6b58d);

			const float x = x0 + padding.x / 2.f;
			const float y = info_bar_y0 + (padding.y / 2.f) + font.ascent;
			char output_string[1024];
			sprintf(output_string, "%s      LN: %llu     COL: %llu", buffer->title.data, view->current_line_number, view->current_column_number);
			immediate_string(output_string, x, y, 0x052329, font);
		}
	}
	immediate_flush();
}