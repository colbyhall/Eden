#include "ui.h"
#include "editor.h"
#include "os.h"
#include "draw.h"

UI_Context ui_context;

UI_ID ui_id_from_pointer(void* pointer) {
	UI_ID result;
	result.owner = (usize)pointer;
	return result;
}

UI_ID zero_id() {
	return {0};
}

void set_active_item_in_ui_context(UI_ID id) {
	if (ui_context.active_item == zero_id()) {
		ui_context.active_item = id;
	}
}

bool ui_button(UI_ID id, const char* text, float x0, float y0, float x1, float y1, const Font& font) {
    const Input_State& input = g_editor.input_state;
	
    bool result = false;
	if (ui_context.active_item == id) {
		if (input.mouse_went_up) {
			if (ui_context.hot_item == id) {
				result = true;
			}
			ui_context.active_item = zero_id();
		}
	} else if (ui_context.hot_item == id) {
		set_active_item_in_ui_context(id);
	}

	const Vector2 mouse_position = input.current_mouse_position;
	if (point_in_rect(mouse_position, x0, y0, x1, y1)) {
		ui_context.hot_item = id;
	} else if (ui_context.hot_item == id) {
		ui_context.hot_item = zero_id();
	}

	Color button_background = 0xFFFFFF;
	if (ui_context.hot_item == id) {
		button_background = 0xAAAAAA;
	}
	immediate_begin();
	immediate_quad(x0, y0, x1, y1, button_background);
    y0 = font.ascent;
	immediate_string(text, x0, y0, 0x000000, font);
	immediate_flush();

	return result;
}

bool ui_scrollbar(UI_ID id, float x, float y, float height, float filled, float* scrolled) {
    const Input_State& input = g_editor.input_state;
    assert(scrolled);

    const float scrollbar_width = 20.f;

    bool result = false;
    if (ui_context.active_item == id) {
        if (input.mouse_went_up) {
            if (ui_context.hot_item == id) {
                result = true;
            }
            ui_context.active_item = zero_id();
        }
    }
    else if (ui_context.hot_item == id) {
        set_active_item_in_ui_context(id);
    }

    const Color background = 0xFFFFFF;
    const Color foreground = 0xCCCCCC;
    const Color foreground_hovered = 0xAAAAAA;

    float x0 = x -= scrollbar_width;
    float y0 = y;
    float x1 = x0 + scrollbar_width;
    float y1 = y + height;

    const Vector2 mouse_position = input.current_mouse_position;
    if (point_in_rect(mouse_position, x0, y0, x1, y1)) {
        ui_context.hot_item = id;
    }
    else if (ui_context.hot_item == id) {
        ui_context.hot_item = zero_id();
    }

    immediate_begin();
    {
        immediate_quad(x0, y0, x1, y1, background);
    }
    {
        
    }

    immediate_flush();

    return result;
}
