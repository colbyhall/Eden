#pragma once

#include <ch_stl/ch_types.h>
#include "keys.h"
#include "editor.h"

/**
 * Basic Concept
 *
 * Each interactive UI item has a unique id
 * This id is used for keeping track of the item was hot and such in the previous frame. 
 */

struct UI_ID {
	usize owner = 0;
	usize index = 0;

	bool operator==(UI_ID right) { return owner == right.owner && index == right.index; }
	bool operator!=(UI_ID right) { return !(*this == right); }
};


UI_ID ui_id_from_pointer(void* pointer);
UI_ID zero_id();

enum UI_Container_Direction {
    CD_VERTICAL,
    CD_HORIZONTAL,
};

struct UI_Container {
    UI_Container_Direction direction;
};

struct UI_Context {
	UI_ID hot_item;
	UI_ID active_item;
};

bool ui_button(UI_ID id, const char* text, float x0, float y0, float x1, float y1, const Font& font = g_editor.loaded_font);
bool ui_scrollbar(UI_ID id, float x, float y, float height, float filled, float* scrolled);