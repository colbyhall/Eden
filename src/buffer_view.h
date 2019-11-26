#pragma once

#include "buffer.h"

const f32 min_width_ratio = 0.2f;

struct Buffer_View {
	Buffer_ID the_buffer = 0;
	f32 width_ratio = 0.5f;

	ssize cursor = -1;
	ssize selection = -1;

	u64 desired_column = 0;

	f32 current_scroll_y = 0.f;
	f32 target_scroll_y = 0.f;

	bool show_cursor = true;
	f32 cursor_blink_time = 0.f;

	CH_FORCEINLINE bool has_selection() const { return cursor != selection; }

	CH_FORCEINLINE void reset_cursor_timer() {
		show_cursor = true;
		cursor_blink_time = 0.f;
	}

	void set_cursor(ssize new_cursor);
	void remove_selection();
	ssize seek_dir(bool left) const;

	void update_desired_column();
	void ensure_cursor_in_view();

	void on_char_entered(u32 c);
	void on_key_pressed(u8 key);
};

void tick_views(f32 dt);

usize push_view(Buffer_ID the_buffer);
usize insert_view(Buffer_ID the_buffer, usize index);
bool remove_view(usize view_index);
Buffer_View* get_view(usize index);
