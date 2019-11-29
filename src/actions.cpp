#include "actions.h"

#include "buffer_view.h"
#include "buffer.h"

void newline() {
	Buffer_View* const view = get_focused_view();
	Buffer* const buffer = find_buffer(view->the_buffer);
	assert(buffer);

	view->remove_selection();
	// @TODO(CHall): NIX or CLRF
	buffer->add_char('\n', view->cursor + 1);
	view->cursor += 1;
	view->selection = view->cursor;
	view->update_desired_column();
	buffer->syntax_dirty = true;
}

void backspace() {
	Buffer_View* const view = get_focused_view();
	Buffer* const buffer = find_buffer(view->the_buffer);
	assert(buffer);

	if (view->cursor <= -1) return;

	buffer->remove_char(view->cursor);
	view->cursor -= 1;
	view->selection -= 1;
	buffer->syntax_dirty = true;
	view->update_desired_column();
}