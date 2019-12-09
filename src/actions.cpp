#include "actions.h"

#include "buffer_view.h"
#include "buffer.h"

void newline() {
	Buffer_View* const view = get_focused_view();
	Buffer* const buffer = find_buffer(view->the_buffer);
	assert(buffer);

	view->remove_selection();

	if (buffer->line_ending == LE_CRLF) {
		buffer->add_char('\r', view->cursor);
		view->cursor += 1;
	}
	buffer->add_char('\n', view->cursor);
	view->cursor += 1;

	view->selection = view->cursor;
	view->update_column_info();
	buffer->syntax_dirty = true;

	view->reset_cursor_timer();
}

void backspace() {
	Buffer_View* const view = get_focused_view();
	Buffer* const buffer = find_buffer(view->the_buffer);
	assert(buffer);

	view->remove_selection();

	if (view->cursor <= 0) return;

	view->cursor = buffer->find_prev_char(view->cursor);
	const u32 c = buffer->get_char(view->cursor);
	buffer->remove_char(view->cursor);

	if (view->cursor > 0) {
		const usize prev_index = buffer->find_prev_char(view->cursor);
		const u32 prev_c = buffer->get_char(prev_index);
		if (c == '\n' && prev_c == '\r') {
			view->cursor = prev_index;
			buffer->remove_char(view->cursor);
		}
	}

	view->selection = view->cursor;
	buffer->syntax_dirty = true;
	view->update_column_info();

	view->reset_cursor_timer();
}

void move_cursor_right() {
	Buffer_View* const view = get_focused_view();
	Buffer* const buffer = find_buffer(view->the_buffer);
	assert(buffer);

	if (view->cursor >= buffer->gap_buffer.count()) return;

	if (view->cursor < buffer->gap_buffer.count() - 1) {
		const u32 c = buffer->get_char(view->cursor);
		const usize next_index = buffer->find_next_char(view->cursor);
		const u32 next_c = buffer->get_char(next_index);
		if (c == '\r' && next_c == '\n') {
			view->cursor = next_index;
		}
	}

	view->cursor = buffer->find_next_char(view->cursor);
	view->selection = view->cursor;
	view->update_column_info(true);
	view->reset_cursor_timer();
}

void move_cursor_left() {
	Buffer_View* const view = get_focused_view();
	Buffer* const buffer = find_buffer(view->the_buffer);
	assert(buffer);

	if (view->cursor <= 0) return;

	view->cursor = buffer->find_prev_char(view->cursor);

	const u32 c = buffer->get_char(view->cursor);
	if (view->cursor > 0) {
		const usize prev_index = buffer->find_prev_char(view->cursor);
		const u32 prev_c = buffer->get_char(prev_index);
		if (c == '\n' && prev_c == '\r') {
			view->cursor = prev_index;
		}
	}

	view->selection = view->cursor;
	view->update_column_info(true);
	view->reset_cursor_timer();
}

void move_cursor_up() {
	Buffer_View* const view = get_focused_view();
	Buffer* const buffer = find_buffer(view->the_buffer);
	assert(buffer);

	const u64 current_line = view->current_line;
	if (current_line <= 0) return;

	const usize line_index = buffer->get_index_from_line(current_line);
	const usize prev_line_index = buffer->get_index_from_line(current_line - 1);

	u32 col_count = 0;
	usize i = prev_line_index;
	for (; i < line_index; i = buffer->find_next_char(i)) {
		const u32 c = buffer->get_char(i);
		if (c == '\r' || c == '\n') break;
		col_count += get_char_column_size(c);
		if (col_count > view->desired_column) break;
	}

	view->cursor = i;
	view->selection = i;

	view->update_column_info();
	view->reset_cursor_timer();
}

void move_cursor_down() {
	Buffer_View* const view = get_focused_view();
	Buffer* const buffer = find_buffer(view->the_buffer);
	assert(buffer);

	const u64 current_line = view->current_line;
	const usize num_lines = buffer->eol_table.count;

	if (current_line + 1 >= num_lines) return;

	const usize next_line_index = buffer->get_index_from_line(current_line + 1);
	const usize next_line_size = buffer->eol_table[current_line + 1];

	u32 col_count = 0;
	usize i = next_line_index;
	for (; i < next_line_index + next_line_size; i = buffer->find_next_char(i)) {
		const u32 c = buffer->get_char(i);
		col_count += get_char_column_size(c);
		if (c == '\r' || c == '\n') break;
		if (col_count > view->desired_column) break;
	}

	view->cursor = i;
	view->selection = i;

	view->update_column_info();
	view->reset_cursor_timer();

}

void save_buffer() {
	Buffer_View* const view = get_focused_view();
	Buffer* const buffer = find_buffer(view->the_buffer);
	assert(buffer);

	buffer->save_file_to_path();
}
