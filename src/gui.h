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
};

CH_FORCEINLINE bool is_point_in_rect(ch::Vector2 p, f32 x0, f32 y0, f32 x1, f32 y1) {
	return p.x >= x0 && p.x <= x1 && p.y >= y0 && p.y <= y1;
}

void gui_label(const ch::String& s, const ch::Color& color, f32 x, f32 y);
CH_FORCEINLINE void gui_label(const char* s, const ch::Color& color, f32 x, f32 y) {
	ch::String real;
	real.data = (char*)s;
	real.count = ch::strlen(s);
	gui_label(real, color, x, y);
}

bool gui_button(f32 x0, f32 y0, f32 x1, f32 y1);

bool gui_button_label(const ch::String& s, f32 x0, f32 y0, f32 x1, f32 y1);
CH_FORCEINLINE bool gui_button_label(const char* s, f32 x0, f32 y0, f32 x1, f32 y1) {
	ch::String real;
	real.data = (char*)s;
	real.count = ch::strlen(s);
	return gui_button_label(real, x0, y0, x1, y1);
}

bool gui_buffer(const struct Buffer& buffer, usize* cursor, usize* selection, bool show_cursor, bool show_line_numbers, bool edit_mode, f32 scroll_y, f32 x0, f32 y0, f32 x1, f32 y1);

void draw_gui();