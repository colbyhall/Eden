#pragma once

#include <ch_stl/string.h>
#include <ch_stl/math.h>

struct Vertical_Layout {
	f32 row_height;
	f32 top_x;
	f32 left_y;
	f32 width;
	f32 at_x;
	f32 at_y;

	Vertical_Layout(f32 _top_x, f32 _left_y, f32 _row_height) : top_x(_top_x), left_y(_left_y), row_height(_row_height), at_x(_top_x), at_y(_left_y) {}

	void row();
	void row(f32 height);
};

struct UI_ID {
	u32 ptr = 0;
	u32 index = 0;

	UI_ID() = default;
	UI_ID(const void* in_ptr) : ptr((u32)in_ptr), index(0) {}
	UI_ID(const void* in_ptr, usize in_index) : ptr((u32)in_ptr), index(in_index) {}

	bool operator==(UI_ID other) const {
		return ptr == other.ptr && index == other.index;
	}

	bool operator!=(UI_ID other) const {
		return !(*this == other);
	}

	operator bool() const { return ptr || index; }
};

CH_FORCEINLINE bool is_point_in_rect(ch::Vector2 p, f32 x0, f32 y0, f32 x1, f32 y1) {
	return p.x >= x0 && p.x <= x1 && p.y >= y0 && p.y <= y1;
}

void gui_label(const ch::String& s, const ch::Color& color, f32 x, f32 y);
CH_FORCEINLINE void gui_label(UI_ID id, const char* s, const ch::Color& color, f32 x, f32 y) {
	ch::String real;
	real.data = (char*)s;
	real.count = ch::strlen(s);
	gui_label(real, color, x, y);
}

bool gui_button(UI_ID id, f32 x0, f32 y0, f32 x1, f32 y1);

bool gui_button_label(UI_ID id, const ch::String& s, f32 x0, f32 y0, f32 x1, f32 y1);
CH_FORCEINLINE bool gui_button_label(UI_ID id, const char* s, f32 x0, f32 y0, f32 x1, f32 y1) {
	ch::String real;
	real.data = (char*)s;
	real.count = ch::strlen(s);
	return gui_button_label(id, real, x0, y0, x1, y1);
}

void tick_gui();