#pragma once

#include "buffer.h"

const f32 min_width_ratio = 0.2f;

struct Buffer_View {
	Buffer_ID the_buffer;
	f32 width_ratio = 0.5f;

	ssize cursor = -1;
	ssize selection = -1;

	f32 current_scroll_y = 0.f;
	f32 target_scroll_y = 0.f;

	bool show_cursor = true;
	f32 cursor_blink_time = 0.f;

	CH_FORCEINLINE bool has_selection() const { return cursor != selection; }

	CH_FORCEINLINE void reset_cursor_timer() {
		show_cursor = true;
		cursor_blink_time = 0.f;
	}

	void on_char_entered(u32 c);
	void on_action_entered(const struct Action_Bind& action);
};

extern Buffer_View* focused_view;
extern Buffer_View* hovered_view;

void tick_views(f32 dt);
void draw_views();

usize push_view(Buffer_ID the_buffer);
usize insert_view(Buffer_ID the_buffer, usize index);
bool remove_view(usize view_index);
Buffer_View* get_view(usize index);
ssize get_view_index(Buffer_View* view);
