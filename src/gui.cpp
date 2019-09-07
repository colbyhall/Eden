#include "gui.h"
#include "editor.h"
#include "draw.h"
#include "input.h"
#include "config.h"

enum Render_Type {
	RT_Quad,
	RT_Border_Quad,
	RT_Text,
	RT_Glyph,
};

struct Render_Command {
	Render_Type type;
	ch::Color color;
	f32 z_index = 9.f;
	union {
		struct {
			f32 x0, y0, x1, y1;
			f32 thickness;
		};
		struct {
			f32 x, y;
			ch::String text;
		};
		struct {
			f32 x, y;
			const Font_Glyph* g;
		};
	};

	Render_Command() {}
};

static ch::Array<Render_Command> commands;

static void push_quad(f32 x0, f32 y0, f32 x1, f32 y1, const ch::Color& color, f32 z_index = 9.f) {
	Render_Command rc;
	rc.type = RT_Quad;
	rc.color = color;
	rc.z_index = z_index;
	rc.x0 = x0;
	rc.y0 = y0;
	rc.x1 = x1;
	rc.y1 = y1;
	commands.push(rc);
}

static void push_quad(ch::Vector2 position, ch::Vector2 size, const ch::Color& color, f32 z_index = 9.f) {
	Render_Command rc;
	rc.type = RT_Quad;
	rc.color = color;
	rc.z_index = z_index;
	rc.x0 = position.x - (size.x / 2.f);
	rc.y0 = position.y + (size.y / 2.f);
	rc.x1 = rc.x0 + size.x;
	rc.y1 = rc.y1 - size.y;
	commands.push(rc);
}

static void push_border_quad(f32 x0, f32 y0, f32 x1, f32 y1, f32 thickness, const ch::Color& color, f32 z_index = 9.f) {
	Render_Command rc;
	rc.type = RT_Border_Quad;
	rc.color = color;
	rc.z_index = z_index;
	rc.x0 = x0;
	rc.y0 = y0;
	rc.x1 = x1;
	rc.y1 = y1;
	rc.thickness = thickness;
	commands.push(rc);
}

static void push_border_quad(ch::Vector2 position, ch::Vector2 size, f32 thickness, const ch::Color& color, f32 z_index = 9.f) {
	Render_Command rc;
	rc.type = RT_Border_Quad;
	rc.color = color;
	rc.z_index = z_index;
	rc.x0 = position.x - (size.x / 2.f);
	rc.y0 = position.y + (size.y / 2.f);
	rc.x1 = rc.x0 + size.x;
	rc.y1 = rc.y1 - size.y;
	rc.thickness = thickness;
	commands.push(rc);
}

static void push_text(const ch::String& text, f32 x, f32 y, const ch::Color& color, f32 z_index = 9.f) {
	Render_Command rc;
	rc.type = RT_Text;
	rc.color = color;
	rc.z_index = z_index;
	rc.text = text;
	rc.x = x;
	rc.y = y - the_font.line_gap;
	commands.push(rc);
}

static void push_text(const tchar* tstr, f32 x, f32 y, const ch::Color& color, f32 z_index = 9.f) {
	Render_Command rc;
	rc.type = RT_Text;
	rc.color = color;
	rc.z_index = z_index;
	rc.text = ch::String(tstr);
	rc.x = x;
	rc.y = y - the_font.line_gap;
	commands.push(rc);
}

static void push_glyph(const Font_Glyph* g, f32 x, f32 y, const ch::Color& color, f32 z_index = 9.f) {
	Render_Command rc;
	rc.type = RT_Glyph;
	rc.color = color;
	rc.z_index = z_index;
	rc.g = g;
	rc.x = x;
	rc.y = y - the_font.line_gap;
	commands.push(rc);
}

/* STYLE */

const f32 border_width = 5.f;

/* GUI */

bool gui_button(const ch::String& s, f32 x0, f32 y0, f32 x1, f32 y1) {
	bool result = false;

	const bool lmb_went_down = was_mouse_button_pressed(CH_MOUSE_LEFT);
	const bool lmb_down = is_mouse_button_down(CH_MOUSE_LEFT);

	const bool is_hovered = is_point_in_rect(current_mouse_position, x0, y0, x1, y1);

	push_quad(x0, y0, x1, y1, ch::white);

	return result;
}

static bool is_point_in_glyph(ch::Vector2 p, const Font_Glyph* g, f32 x, f32 y) {
	const f32 x0 = x;
	const f32 y0 = y;
	const f32 x1 = x0 + g->advance;
	const f32 y1 = y0 + (f32)the_font.size;

	return is_point_in_rect(p, x0, y0, x1, y1);
}

void gui_text_edit(const ch::Gap_Buffer<u32>& gap_buffer, ssize* cursor, ssize* selection, bool show_cursor, f32 x0, f32 y0, f32 x1, f32 y1) {
	const ch::Vector2 mouse_pos = current_mouse_position;
	const bool was_lmb_pressed = was_mouse_button_pressed(CH_MOUSE_LEFT);
	const bool is_lmb_down = is_mouse_button_down(CH_MOUSE_LEFT);

	const Config config = get_config();

	push_quad(x0, y0, x1, y1, config.background_color);

	const f32 font_height = the_font.size;
	const Font_Glyph* space_glyph = the_font[' '];
	const Font_Glyph* unknown_glyph = the_font['?'];
	
	{
		const f32 starting_x = x0;
		const f32 starting_y = y0;
		f32 x = starting_x;
		f32 y = starting_y;
		u64 line_number = 1;

		for (usize i = 0; i < gap_buffer.count(); i += 1) {
			const u32 c = gap_buffer[i];
			ch::Color color = config.foreground_color;

			if (c == ch::eol) {
				x = starting_x;
				y += font_height;
				continue;
			}

			if (c == '\t') {
				x += space_glyph->advance * config.tab_width;
				continue;
			}

			const Font_Glyph* g = the_font[c];
			if (!g) {
				color = ch::magenta;
				g = unknown_glyph;
			}

			const bool is_in_cursor = *cursor + 1 == i && show_cursor;

			// @DEBUG(CHall): This is for debugging
			if (is_point_in_glyph(mouse_pos, g, x, y)) {
				push_quad(x, y, x + g->advance, y + font_height, config.line_number_text_color);
			}

			if (is_in_cursor) {
				push_quad(x, y, x + g->advance, y + font_height, config.cursor_color);
			}
			
			push_glyph(g, x, y, color);
			x += g->advance;
		}

		if (*cursor + 1 == gap_buffer.count() && show_cursor) push_quad(x, y, x + space_glyph->advance, y + font_height, config.cursor_color);
	}
}

void draw_gui() {
	the_font.bind();
	refresh_shader_transform();
	immediate_begin();
	const f32 font_height = (f32)the_font.size;
	for (const Render_Command& it : commands) {
		switch (it.type) {
		case RT_Quad:
			immediate_quad(it.x0, it.y0, it.x1, it.y1, it.color, it.z_index);
			break;
		case RT_Border_Quad:
			// draw_border_quad(it.x0, it.y0, it.x1, it.y1, it.thickness, it.color, it.z_index);
			break;
		case RT_Text:
			immediate_string(it.text, the_font, it.x, it.y, it.color, it.z_index);
			break;
		case RT_Glyph:
			immediate_glyph(*it.g, the_font, it.x, it.y, it.color, it.z_index);
			break;
		}

	}
	immediate_flush();

	commands.count = 0;
}