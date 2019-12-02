#include "buffer_view.h"
#include "input.h"
#include "editor.h"
#include "config.h"
#include "gui.h"

#include <ch_stl/math.h>

static ch::Array<Buffer_View> views;
static usize focused_view;

void ensure_all_cursors_visible() {
    for (usize i = 0; i < views.count; i++) {
        views[i].ensure_cursor_in_view();
    }
}

f32 get_view_width(f32 viewport_width, usize i) {
    if (i < views.count - 1) {
        return viewport_width * views[i].width_ratio;
    } else {
        f32 sum_of_widths = 0;
        for (usize i = 0; i < views.count - 1; i++) {
            sum_of_widths += views[i].width_ratio;
        }
        return viewport_width * (1 - sum_of_widths);
    }
}
u64 get_view_columns(f32 viewport_width, usize i) {
    Buffer_View* view = &views[i];
    Buffer* buffer = find_buffer(view->the_buffer);
	assert(buffer);
    u64 result = (u64)(get_view_width(viewport_width, i) / the_font[' ']->advance);
    result -= (ch::get_num_digits(buffer->eol_table.count) + 1);
    //if (result > (ch::get_num_digits(buffer->eol_table.count) + 1)) {
    //    result -= (ch::get_num_digits(buffer->eol_table.count) + 1);
    //}
    return result;
}

void Buffer_View::set_cursor(ssize new_cursor) {
	Buffer* buffer = find_buffer(the_buffer);
	assert(buffer);
	assert(new_cursor >= -1 && new_cursor < (ssize)buffer->gap_buffer.count());

	cursor = new_cursor;
	selection = new_cursor;
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
			const char c = buffer->gap_buffer[result];
			if ((ch::is_symbol(c) || ch::is_whitespace(c)) && found_letter) {
				break;
			} else if (ch::is_letter(c)) {
				found_letter = true;
			}
		}
	} else {
		for (; result < (ssize)buffer->gap_buffer.count() - 1; result += 1) {
			const char c = buffer->gap_buffer[result];
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

void Buffer_View::update_desired_column() {
	Buffer* buffer = find_buffer(the_buffer);
	assert(buffer);

	const u64 current_line = buffer->get_line_from_index(cursor + 1);
	const u64 line_index = buffer->get_index_from_line(current_line);
	desired_column = (cursor + 1) - line_index;
}

void Buffer_View::ensure_cursor_in_view() {
    Buffer* buffer = find_buffer(the_buffer);
	assert(buffer);

	return;

    u64 max_line_width = get_view_columns((f32)the_window.get_viewport_size().ux, this - views.begin()); // @Hack: this "self-contained view acting only on itself" abstraction is bad

    f32 view_height = (f32)the_window.get_viewport_size().uy;
    f32 font_height = (f32)the_font.size;

    f32 cursor_y = (buffer->get_wrapped_line_from_index(cursor + 1, max_line_width)) * font_height; // @Todo: need to compute the exact value for wrapped line scrolling
    // print_to_messages("%llu\n", max_line_width);

    if (target_scroll_y > cursor_y - font_height * 2) {
	    target_scroll_y = cursor_y - font_height * 2;
    } else if (target_scroll_y < cursor_y + font_height * 2 - view_height) {
	    target_scroll_y = cursor_y + font_height * 2 - view_height;
    }
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

	defer(ensure_cursor_in_view());

    buffer->syntax_dirty = true;
}

static u64 seek_end_of_line(Buffer* b, u64 begin) {
    u64 i = begin;
    while (i >= 0 && i < b->gap_buffer.count() && b->gap_buffer[i] != ch::eol) {
        i += 1;
    }
    return i;
}
static u64 seek_begin_of_line(Buffer* b, u64 begin) {
    u64 i = begin;
    while (i >= 0 && i < b->gap_buffer.count() && b->gap_buffer[i] != ch::eol) {
        i -= 1;
    }
    return i;
}

void tick_views(f32 dt) {
	f32 x = 0.f;
	const ch::Vector2 viewport_size = the_window.get_viewport_size();
	const f32 viewport_width = (f32)viewport_size.ux;
	const f32 viewport_height = (f32)viewport_size.uy;
	const ch::Vector2 mouse_pos = current_mouse_position;

    if (!viewport_width || !viewport_height) return;

	const Config& config = get_config();

	for (usize i = 0; i < views.count; i += 1) {
		Buffer_View* view = &views[i];

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
		const f32 x1 = x0 + get_view_width(viewport_width, i);
		const f32 y1 = viewport_height;

		Buffer* the_buffer = find_buffer(view->the_buffer);
        assert(the_buffer);

		if (is_point_in_rect(mouse_pos, x0, y0, x1, y1) &&
			!(view->target_scroll_y == 0.f && current_mouse_scroll_y > 0.f)) {
			view->target_scroll_y -= current_mouse_scroll_y;
		}

		view->current_scroll_y = ch::interp_to(view->current_scroll_y, view->target_scroll_y, dt, config.scroll_speed);

        parsing::parse_cpp(find_buffer(view->the_buffer));

		f32 max_scroll_y = 0.f;
        if (the_buffer) {
		    if (gui_buffer(*the_buffer, &view->cursor, &view->selection, view->show_cursor, config.show_line_numbers, focused_view == i, view->current_scroll_y, &max_scroll_y, x0, y0, x1, y1)) {
		    	view->reset_cursor_timer();
		    	view->update_desired_column();
                focused_view = i;
		    }
        }

		if (view->target_scroll_y > max_scroll_y) {
			view->target_scroll_y = max_scroll_y;
		}

		if (view->target_scroll_y < 0.f) {
			view->target_scroll_y = 0.f;
		}

		x += get_view_width(viewport_width, i);
	}
}

Buffer_View* get_focused_view() {
	if (views.count > 0) {
		assert(focused_view < views.count);
		return &views[focused_view];
	}
	return nullptr;
}

usize push_view(Buffer_ID the_buffer) {
	Buffer_View view = {};
	view.the_buffer = the_buffer;
    usize result = views.push(view);
	return result;
}

usize insert_view(Buffer_ID the_buffer, usize index) {
	Buffer_View view = {};
	view.the_buffer = the_buffer;
	views.insert(view, index);
	return index;
}

bool remove_view(usize view_index) {
	assert(view_index < views.count);
	views.remove(view_index);
	return true;
}

Buffer_View* get_view(usize index) {
	return &views[index];
}