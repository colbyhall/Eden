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

	bool found_letter = false;
	ssize result = cursor + (!left * 2);
	if (left) {
		for (; result > -1; result -= 1) {
			const tchar c = buffer->gap_buffer[result];
			if ((ch::is_symbol(c) || ch::is_whitespace(c)) && found_letter) {
				break;
			} else if (ch::is_letter(c)) {
				found_letter = true;
			}
		}
	} else {
		for (; result < (ssize)buffer->gap_buffer.count() - 1; result += 1) {
			const tchar c = buffer->gap_buffer[result];
			if ((ch::is_symbol(c) || ch::is_whitespace(c)) && found_letter) {
				result -= 1;
				break;
			} else if (ch::is_letter(c)) {
				found_letter = true;
			}
		}
	}

	return result;
}

void Buffer_View::set_focused() {
	focused_view = this;
}

void Buffer_View::update_desired_column() {
	Buffer* buffer = find_buffer(the_buffer);
	assert(buffer);

	const u64 current_line = buffer->get_line_from_index(cursor + 1);
	const u64 line_index = buffer->get_index_from_line(current_line);
	desired_column = (cursor + 1) - line_index;
}

void Buffer_View::on_char_entered(u32 c) {
	Buffer* buffer = find_buffer(the_buffer);
	assert(buffer);

	remove_selection();
	buffer->add_char(c, cursor + 1);
	cursor += 1;
	selection = cursor;

	update_desired_column();

	reset_cursor_timer();

    buffer->syntax_dirty = true;
}

void Buffer_View::on_key_pressed(u8 key) {
	Buffer* buffer = find_buffer(the_buffer);
	assert(buffer);

	const bool ctrl_pressed = is_key_down(CH_KEY_CONTROL);
	const bool shift_pressed = is_key_down(CH_KEY_SHIFT);
	const bool alt_pressed = is_key_down(CH_KEY_ALT);

	reset_cursor_timer();
	switch (key) {
	case CH_KEY_ENTER:
		// @TODO(CHall): Detect if nix or CRLF
		remove_selection();
		buffer->add_char('\n', cursor + 1);
		cursor += 1;
		selection = cursor;
		update_desired_column();
		buffer->syntax_dirty = true;
		break;
	case CH_KEY_BACKSPACE: 
		if (has_selection()) {
			remove_selection();
		}

		if (cursor > -1) {
			if (ctrl_pressed) {
				selection = seek_dir(true);
				remove_selection();
			} else {
				buffer->remove_char(cursor);
				cursor -= 1;
				selection = cursor;
			}

            buffer->syntax_dirty = true;
			update_desired_column();
		}
		break;
	case CH_KEY_DELETE:
		if (has_selection()) {
			remove_selection();
		}

		if (cursor < (ssize)buffer->gap_buffer.count() - 1) {
			if (ctrl_pressed) {
				selection = seek_dir(false);
				remove_selection();
			} else {
				buffer->remove_char(cursor + 1);
			}

            buffer->syntax_dirty = true;
			update_desired_column();
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
			update_desired_column();
		}
		break;
	case CH_KEY_RIGHT:
		if (alt_pressed) {
			ssize index = get_view_index(this);
			index += 1;

			if (index > (ssize)views.count - 1) index = 0;

			views[index]->set_focused();
		} else {
			if (cursor < (ssize)buffer->gap_buffer.count() - 1) {
				if (ctrl_pressed) {
					set_cursor(seek_dir(false));
				} else {
					set_cursor(cursor + 1);
				}
			} else {
				set_cursor(cursor);
			}
			update_desired_column();
		}
		break;
	case CH_KEY_UP: {
		const u64 current_line = buffer->get_line_from_index(cursor + 1);
		if (current_line > 0) {
			const u64 target_line = current_line - 1;
			const u64 line_index = buffer->get_index_from_line(target_line);
			const u64 line_size = buffer->eol_table[target_line];
			u64 new_index = line_index;
			if (desired_column >= line_size) {
				new_index += line_size - 1;
			} else {
				new_index += desired_column;
			}
			set_cursor(new_index - 1);
		}

	} break;
	case CH_KEY_DOWN: {
		const u64 current_line = buffer->get_line_from_index(cursor + 1);
		if (current_line < buffer->eol_table.count - 1) {
			const u64 target_line = current_line + 1;
			const u64 line_index = buffer->get_index_from_line(target_line);
			const u64 line_size = buffer->eol_table[target_line];
			u64 new_index = line_index;
			if (desired_column >= line_size) {
				new_index += line_size - 1;
			}
			else {
				new_index += desired_column;
			}
			set_cursor(new_index - 1);
		}
	} break;
	}
}

void tick_views(f32 dt) {
	f32 x = 0.f;
	const ch::Vector2 viewport_size = the_window.get_viewport_size();
	const f32 viewport_width = (f32)viewport_size.ux;
	const f32 viewport_height = (f32)viewport_size.uy;
	const ch::Vector2 mouse_pos = current_mouse_position;

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

		if (is_point_in_rect(mouse_pos, x0, y0, x1, y1)) {
			view->target_scroll_y -= current_mouse_scroll_y;

			if (view->target_scroll_y < 0.f) view->target_scroll_y = 0.f;
		}
        
        parsing::parse_cpp(find_buffer(views[0]->the_buffer));

		f32 max_scroll_y = 0.f;
		if (gui_buffer(*the_buffer, &view->cursor, &view->selection, view->show_cursor, config.show_line_numbers, focused_view == view, view->current_scroll_y, &max_scroll_y, x0, y0, x1, y1)) {
			view->reset_cursor_timer();
			view->update_desired_column();
			view->set_focused();
		}

		if (view->target_scroll_y > max_scroll_y) {
			view->target_scroll_y = max_scroll_y;
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
