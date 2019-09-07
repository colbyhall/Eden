#pragma once

#include <ch_stl/string.h>
#include <ch_stl/math.h>
#include <ch_stl/gap_buffer.h>

CH_FORCEINLINE bool is_point_in_rect(ch::Vector2 p, f32 x0, f32 y0, f32 x1, f32 y1) {
	return p.x >= x0 && p.x <= x1 && p.y >= y0 && p.y <= y1;
}

bool gui_button(const ch::String& s, f32 x0, f32 y0, f32 x1, f32 y1);
CH_FORCEINLINE bool gui_button(const tchar* s, f32 x0, f32 y0, f32 x1, f32 y1) {
	ch::String real;
	real.data = (tchar*)s;
	real.count = ch::strlen(s);
	return gui_button(real, x0, y0, x1, y1);
}

void gui_text_edit(const ch::Gap_Buffer<u32>& gap_buffer, ssize* cursor, ssize* selection, bool show_cursor, f32 x0, f32 y0, f32 x1, f32 y1);

void draw_gui();