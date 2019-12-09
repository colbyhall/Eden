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
	view->update_desired_column();
	buffer->syntax_dirty = true;

	view->reset_cursor_timer();
}

void backspace() {
	Buffer_View* const view = get_focused_view();
	Buffer* const buffer = find_buffer(view->the_buffer);
	assert(buffer);

	if (view->cursor <= 0) return;

	view->cursor = buffer->find_prev_char(view->cursor);
	view->selection = view->cursor;
	buffer->remove_char(view->cursor);

	buffer->syntax_dirty = true;
	view->update_desired_column();

	view->reset_cursor_timer();
}

void move_cursor_right() {
	Buffer_View* const view = get_focused_view();
	Buffer* const buffer = find_buffer(view->the_buffer);
	assert(buffer);

	if (view->cursor >= buffer->gap_buffer.count()) return;

	view->cursor = buffer->find_next_char(view->cursor);
	view->selection = view->cursor;
	view->update_desired_column();
	view->reset_cursor_timer();
}

void move_cursor_left() {
	Buffer_View* const view = get_focused_view();
	Buffer* const buffer = find_buffer(view->the_buffer);
	assert(buffer);

	if (view->cursor <= 0) return;

	view->cursor = buffer->find_prev_char(view->cursor);
	view->selection = view->cursor;
	view->update_desired_column();
	view->reset_cursor_timer();
}
