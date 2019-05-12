#pragma once

#include "types.h"
#include "keys.h"
#include "editor.h"

/**
 * Basic Concept
 *
 * Each interactive UI item has a unique id
 * This id is used for keeping track of the item was hot and such in the previous frame. 
 */

struct UI_ID {
	size_t owner = 0;
	size_t index = 0;

	bool operator==(UI_ID right) { return owner == right.owner && index == right.index; }
	bool operator!=(UI_ID right) { return !(*this == right); }
};


UI_ID ui_id_from_pointer(void* pointer);
UI_ID zero_id();

struct UI_Context {
	UI_ID hot_item;
	UI_ID active_item;
};

bool ui_button(UI_ID id, const Input_State& input, const char* text, const Font& font, float x0, float y0, float x1, float y1);