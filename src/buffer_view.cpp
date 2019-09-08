#include "buffer_view.h"
#include "input.h"
#include "editor.h"
#include "config.h"
#include "gui.h"

#include <ch_stl/math.h>

Buffer_View* focused_view;
Buffer_View* hovered_view;
ch::Array<Buffer_View*> views;

void Buffer_View::on_char_entered(u32 c) {
	Buffer* buffer = find_buffer(the_buffer);
	assert(buffer);

	if (c == '\r') c = ch::eol;

	reset_cursor_timer();
	switch (c) {
	case CH_KEY_BACKSPACE:
		if (cursor > -1) {
			buffer->remove_char(cursor);
			cursor -= 1;
			selection -= 1;
		}
		break;
	default:
		buffer->add_char(c, cursor + 1);
		cursor += 1;
		selection += 1;
		break;
	}
}

void Buffer_View::on_action_entered(const Action_Bind& action) {

}

void tick_views(f32 dt) {
	f32 x = 0.f;
	const ch::Vector2 viewport_size = the_window.get_viewport_size();
	const f32 viewport_width = (f32)viewport_size.ux;
	const f32 viewport_height = (f32)viewport_size.uy;

	for (usize i = 0; i < views.count; i += 1) {
		Buffer_View* view = views[i];

		view->current_scroll_y = ch::interp_to(view->current_scroll_y, view->target_scroll_y, dt, get_config().scroll_speed);

		u32 blink_time;
		if (!ch::get_caret_blink_time(&blink_time)) {
			view->cursor_blink_time = 0.f;
			view->show_cursor = true;
		}
		view->cursor_blink_time += dt;
		if (view->cursor_blink_time > (f32)blink_time / 1000.f) {
			view->show_cursor = !view->show_cursor;
			view->cursor_blink_time = 0.f;
		}

		const f32 x0 = x;
		const f32 y0 = 0.f;
		const f32 x1 = (i == views.count - 1) ? viewport_width - x : viewport_width * view->width_ratio;
		const f32 y1 = viewport_height;

		Buffer* the_buffer = find_buffer(view->the_buffer);

		if (gui_text_edit(the_buffer->gap_buffer, &view->cursor, &view->selection, view->show_cursor, the_buffer->eol_table.count, get_config().show_line_numbers, x0, y0, x1, y1)) {
			view->reset_cursor_timer();
		}

		x += x1 - x0;
	}
}

usize push_view(Buffer_ID the_buffer) {
	Buffer_View* view = ch_new Buffer_View;
	view->the_buffer = the_buffer;
	if (!focused_view) focused_view = view;
	return views.push(view);
}

usize insert_view(Buffer_ID the_buffer, usize index) {
	Buffer_View* view = ch_new Buffer_View;
	view->the_buffer = the_buffer;
	views.insert(view, index);
	return index;
}

bool remove_view(usize view_index) {
	assert(view_index < views.count);
	Buffer_View* view = views[view_index];
	ch_delete view;
	views.remove(view_index);
	return true;
}

Buffer_View* get_view(usize index) {
	return views[index];
}

ssize get_view_index(Buffer_View* view) {
	return views.find(view);
}
