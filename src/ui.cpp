#include "ui.h"
#include "editor.h"
#include "os.h"
#include "draw.h"

UI_Context ui_context;

UI_ID ui_id_from_pointer(void* pointer) {
	UI_ID result;
	result.owner = (size_t)pointer;
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

bool ui_button(UI_ID id, const Input_State& input, const char* text, const Font& font, float x0, float y0, float x1, float y1) {
	bool result = false;
	if (ui_context.active_item == id) {
		if (input.mouse_went_down) {
			if (ui_context.hot_item == id) {
				result = true;
			}
			ui_context.active_item = zero_id();
		}
	} else if (ui_context.hot_item == id) {
		set_active_item_in_ui_context(id);
	}

	const Vector2 mouse_position = v2(0.f);
	if (mouse_position.x >= x0 && mouse_position.x <= x1 && mouse_position.y >= y0 && mouse_position.y <= y1) {
		ui_context.hot_item = id;
	} else if (ui_context.hot_item == id) {
		ui_context.hot_item = zero_id();
	}

	Color button_background = 0xFFFFFF;
	if (ui_context.hot_item == id) {
		button_background = 0xAAAAAA;
	}
	immediate_quad(x0, y0, x1, y1, button_background);
	immediate_string(text, x0, y0, 0x000000, font);

	return result;
}