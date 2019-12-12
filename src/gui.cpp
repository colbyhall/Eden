#include "gui.h"
#include "editor.h"
#include "buffer.h"
#include "draw.h"
#include "input.h"
#include "config.h"

#include "parsing.h"

#include <ch_stl/time.h>

struct UI_Context {
	UI_ID focused_id;
	UI_ID hovered_id;

	void set_focused(UI_ID id) {
		if (!focused_id) focused_id = id;
	}
};

static UI_Context ui_context;

/* STYLE */

const f32 border_width = 5.f;

/* LAYOUT */

void Vertical_Layout::row() {
	at_y += row_height;
}

void Vertical_Layout::row(f32 height) {
	at_y += height;
}

/* GUI */

void gui_label(const ch::String& s, const ch::Color& color, f32 x, f32 y) {
	const ch::Vector2 draw_size = get_string_draw_size(s, the_font);
	const ch::Vector2 padding = 5.f;

	const f32 x0 = x;
	const f32 y0 = y;
	const f32 x1 = x0 + draw_size.x + padding.x;
	const f32 y1 = y0 + draw_size.y + padding.y;
	imm_quad(x0, y0, x1, y1, ch::white);
	imm_string(s, the_font, x + padding.x / 2.f, y + padding.y / 2.f, color);
}

bool gui_button(UI_ID id, f32 x0, f32 y0, f32 x1, f32 y1) {
	bool result = false;

	const bool lmb_went_down = was_mouse_button_pressed(CH_MOUSE_LEFT);
	const bool lmb_went_up = was_mouse_button_released(CH_MOUSE_LEFT);

	if (id == ui_context.focused_id) {
		if (lmb_went_up) {
			if (id == ui_context.focused_id) result = true;
			ui_context.focused_id = {};
		}
	} else if (id == ui_context.hovered_id) {
		if (lmb_went_down) ui_context.set_focused(id);
	}

	const bool is_hovered = is_point_in_rect(current_mouse_position, x0, y0, x1, y1);
	if (is_hovered) {
		ui_context.hovered_id = id;
	}

	if (id == ui_context.hovered_id) {
		imm_quad(x0, y0, x1, y1, get_config().background_color);
	} else {
		imm_quad(x0, y0, x1, y1, get_config().foreground_color);
	}

	return result;
}

// @TODO: Finish
bool gui_button_label(UI_ID id, const ch::String& s, f32 x0, f32 y0, f32 x1, f32 y1) {
	bool result = false;

	const bool lmb_went_down = was_mouse_button_pressed(CH_MOUSE_LEFT);
	const bool lmb_down = is_mouse_button_down(CH_MOUSE_LEFT);

	const bool is_hovered = is_point_in_rect(current_mouse_position, x0, y0, x1, y1);

	imm_quad(x0, y0, x1, y1, get_config().foreground_color);

	return result;
}

void tick_gui() {
	ui_context.hovered_id = {};
}
