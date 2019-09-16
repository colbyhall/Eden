#include "buffer_view.h"
#include "input.h"
#include "editor.h"
#include "config.h"
#include "gui.h"

#include <ch_stl/math.h>

Buffer_View* focused_view;
ch::Array<Buffer_View*> views;

void Buffer_View::set_cursor(ssize new_cursor) {
	Buffer* buffer = find_buffer(the_buffer);
	assert(buffer);
	assert(new_cursor >= -1 && new_cursor < (ssize)buffer->gap_buffer.count());

	cursor = new_cursor;

	const bool shift_down = is_key_down(CH_KEY_SHIFT);
	if (!shift_down) {
		selection = cursor;	
	}
}

void Buffer_View::remove_selection() {
	if (!has_selection()) return;

	Buffer* buffer = find_buffer(the_buffer);
	assert(buffer);

	if (cursor > selection) {
		for (ssize i = cursor; i > selection; i--) {
			buffer->gap_buffer.remove_at_index(i);
		}
		cursor = selection;
	}
	else {
		for (ssize i = selection; i > cursor; i--) {
			if (i < (ssize)buffer->gap_buffer.count()) {
				buffer->gap_buffer.remove_at_index(i);
			}
		}
		selection = cursor;
	}

	buffer->refresh_eol_table();
}

ssize Buffer_View::seek_dir(bool left) const {
	Buffer* buffer = find_buffer(the_buffer);
	assert(buffer);

	ssize result = cursor + (!left * 2);
	if (left) {
		for (; result > -1; result -= 1) {
			const tchar c = buffer->gap_buffer[result];
			if (ch::is_symbol(c) || ch::is_whitespace(c)) {
				result -= 1;
				break;
			}
		}
	}
	else {
		for (; result < (ssize)buffer->gap_buffer.count() - 1; result += 1) {
			const tchar c = buffer->gap_buffer[result];
			if (ch::is_symbol(c) || ch::is_whitespace(c)) {
				result -= 1;
				break;
			}
		}
	}

	return result;
}

void Buffer_View::set_focused() {
	focused_view = this;
}

void Buffer_View::on_char_entered(u32 c) {
	Buffer* buffer = find_buffer(the_buffer);
	assert(buffer);

	if (c == '\r') c = ch::eol;

	if ((c < 32 || c > 126) && c != ch::eol) return;

	remove_selection();
	buffer->add_char(c, cursor + 1);
	cursor += 1;
	selection = cursor;

	reset_cursor_timer();
}

void Buffer_View::on_key_pressed(u8 key) {
	Buffer* buffer = find_buffer(the_buffer);
	assert(buffer);

	const bool ctrl_pressed = is_key_down(CH_KEY_CONTROL);
	const bool shift_pressed = is_key_down(CH_KEY_SHIFT);
	const bool alt_pressed = is_key_down(CH_KEY_ALT);

	reset_cursor_timer();
	switch (key) {
	case CH_KEY_BACKSPACE: 
		if (cursor > -1) {
			if (ctrl_pressed) {
				selection = seek_dir(true);
				remove_selection();
			} else {
				buffer->remove_char(cursor);
				cursor -= 1;
				selection = cursor;
			}
		}
		break;
	case CH_KEY_DELETE:
		if (cursor < (ssize)buffer->gap_buffer.count() - 1) {
			if (ctrl_pressed) {
				selection = seek_dir(false);
				remove_selection();
			} else {
				buffer->remove_char(cursor + 1);
			}
		}
		break;
	case CH_KEY_LEFT:
		if (alt_pressed) {
			ssize index = get_view_index(this);
			index -= 1;

			if (index < 0) index = views.count - 1;

			views[index]->set_focused();
		} else {
			if (cursor > -1) {
				if (ctrl_pressed) {
					set_cursor(seek_dir(true));
				} else {
					set_cursor(cursor - 1);
				}
			} else {
				set_cursor(cursor);
			}
		}
		break;
	case CH_KEY_RIGHT:
		if (alt_pressed) {
			ssize index = get_view_index(this);
			index += 1;

			if (index > (ssize)views.count - 1) index = 0;

			views[index]->set_focused();
		}
		else {
			if (cursor < (ssize)buffer->gap_buffer.count() - 1) {
				if (ctrl_pressed) {
					set_cursor(seek_dir(false));
				} else {
					set_cursor(cursor + 1);
				}
			} else {
				set_cursor(cursor);
			}
		}
		break;
	}
}

void tick_views(f32 dt) {
	f32 x = 0.f;
	const ch::Vector2 viewport_size = the_window.get_viewport_size();
	const f32 viewport_width = (f32)viewport_size.ux;
	const f32 viewport_height = (f32)viewport_size.uy;

	const Config& config = get_config();

	for (usize i = 0; i < views.count; i += 1) {
		Buffer_View* view = views[i];

		view->current_scroll_y = ch::interp_to(view->current_scroll_y, view->target_scroll_y, dt, config.scroll_speed);

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
		const f32 x1 = x0 + ((i == views.count - 1) ? viewport_width - x : viewport_width * view->width_ratio);
		const f32 y1 = viewport_height;

		Buffer* the_buffer = find_buffer(view->the_buffer);

        parsing::parse_cpp(the_buffer);

		if (gui_buffer(*the_buffer, &view->cursor, &view->selection, view->show_cursor, config.show_line_numbers, focused_view == view, x0, y0, x1, y1)) {
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
