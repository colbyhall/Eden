#pragma once

#include <ch_stl/string.h>
#include <ch_stl/math.h>

#include "buffer.h"

CH_FORCEINLINE bool is_point_in_rect(ch::Vector2 p, f32 x0, f32 y0, f32 x1, f32 y1) {
	return p.x >= x0 && p.x <= x1 && p.y >= y0 && p.y <= y1;
}

struct GUI_ID {
	u64 owner = 0;
	u64 index = 0;

	GUI_ID() = default;
	GUI_ID(void* ptr) : owner((u64)ptr) {}


	CH_FORCEINLINE bool operator==(const GUI_ID& id) {
		return owner == id.owner && index == id.index;
	}

	CH_FORCEINLINE bool operator!=(const GUI_ID& id) { return !(*this == id); }
};

const GUI_ID zero_id;

bool gui_button(GUI_ID id, const ch::String& s, f32 x0, f32 y0, f32 x1, f32 y1);
CH_FORCEINLINE bool gui_button(GUI_ID id, const tchar* s, f32 x0, f32 y0, f32 x1, f32 y1) {
	ch::String real;
	real.data = (tchar*)s;
	real.count = ch::strlen(s);
	return gui_button(id, real, x0, y0, x1, y1);
}
