#include "gui.h"
#include "editor.h"
#include "draw.h"
#include "input.h"

GUI_ID hovered_id;
GUI_ID active_id;

/* STYLE */

const f32 border_width = 5.f;

void set_active_id(GUI_ID id) {
	if (active_id == zero_id) {
		active_id = id;
	}
}

bool gui_button(GUI_ID id, const ch::String& s, f32 x0, f32 y0, f32 x1, f32 y1) {
	bool result = false;
	if (active_id == id) {
		if (!lmb_down && lmb_was_down) {
			if (hovered_id == id) {
				result = true;
			}
			active_id = zero_id;
		}
	} else if (hovered_id == id && lmb_down && !lmb_was_down) {
		set_active_id(id);
	}

	if (is_point_in_rect(current_mouse_position, x0, y0, x1, y1)) {
		hovered_id = id;
	} else if (hovered_id == id) {
		hovered_id = zero_id;
	}

	// @TODO(CHall): Button graphics
	draw_quad(x0, y0, x1, y1, ch::white);	

	return result;
}
