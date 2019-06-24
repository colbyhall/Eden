#include "buffer.h"
#include "parsing.h"
#include "os.h"
#include "draw.h"
#include "font.h"
#include "keys.h"
#include "editor.h"
#include "input.h"

#include <ch_stl/ch_defer.h>

#include <stdio.h>
#include <ch_stl/ch_stl.h>

u32& Buffer::operator[](usize index) {
	assert(index < get_count(*this));

	if (data + index < gap) {
		return data[index];
	}
	else {
		return data[index + gap_size];
	}
}

u32 Buffer::operator[](usize index) const {
	assert(index < get_count(*this));

	if (data + index < gap) {
		return data[index];
	}
	else {
		return data[index + gap_size];
	}
}

Buffer make_buffer(Buffer_ID id) {
    Buffer result = {0};
	result.id = id;
	return result;
}

void destroy_buffer(Buffer* buffer) {
    buffer->eol_table.destroy();
    buffer->syntax.destroy();
    ch::free(buffer->data);
}

bool buffer_load_from_file(Buffer* buffer, const char* path) {
	FILE* file = fopen(path, "rb");
	if (!file) {
		return false;
	}
	
	buffer->path = path;
	fseek(file, 0, SEEK_END);
	const usize file_size = ftell(file);
	fseek(file, 0, SEEK_SET);

	buffer->allocated = file_size + DEFAULT_GAP_SIZE;
	buffer->data = (u32*)ch::malloc(buffer->allocated * sizeof(u32));

    usize chars_read = 0;

	usize extra = 0;
	u32* current_char = buffer->data;
	usize line_size = 0;
	while (current_char != buffer->data + file_size) {
		const int c_ = fgetc(file);
        if (c_ == EOF) break; // Failure.

        chars_read += 1;
        const u32 c = c_;

		if (c == '\r') {
			extra += 1;
			continue;
		} 
		*current_char = (u32)c;
		current_char += 1;
		line_size += 1;
		if (c == '\n'){
			buffer->eol_table.push(line_size);
			line_size = 0;
		}
	}
    // @Cleanup: why does + 1 make things work here?
   // array_add(&buffer->eol_table, line_size - extra);
	fclose(file);

	buffer->gap = buffer->data + file_size - extra;
	buffer->gap_size = DEFAULT_GAP_SIZE + extra;

	// @NOTE(Colby): Basic file name parsing
	for (usize i = buffer->path.count - 1; i >= 0; i--) {
		if (buffer->path[i] == '.') {
			buffer->language.data = buffer->path.data + i + 1;
			buffer->language.count = buffer->path.count - i - 1;
			buffer->language.allocated = buffer->language.count;
		}

		if (buffer->path[i] == '/' || buffer->path[i] == '\\') {
			buffer->title.data = buffer->path.data + i + 1;
			buffer->title.count = buffer->path.count - i - 1;
			break;
		}
	}

    buffer->syntax_is_dirty = true;
	return true;
}

void buffer_init_from_size(Buffer* buffer, usize size) {
	if (size < DEFAULT_GAP_SIZE) size = DEFAULT_GAP_SIZE;

	buffer->allocated = size;
	buffer->data = (u32*)ch::malloc(size * sizeof(u32));

	buffer->gap = buffer->data;
	buffer->gap_size = size;
	buffer->eol_table.push((usize)0);

	buffer->language = "cpp";
}

void buffer_resize(Buffer* buffer, usize new_gap_size) {
	// @NOTE(Colby): Check if we have been initialized
    if (!buffer->data) {
        buffer_init_from_size(buffer, new_gap_size);
        return;
    }
	assert(buffer->data);
	assert(buffer->gap_size == 0);

	const usize old_size = buffer->allocated;
	const usize new_size = old_size + new_gap_size;

	u32 *old_data = buffer->data;
	u32 *new_data = (u32 *)ch::realloc(old_data, new_size * sizeof(u32));
	if (!new_data) {
		assert(false);
	}

	buffer->data = new_data;
	buffer->allocated = new_size;
	buffer->gap = (buffer->data + old_size);
	buffer->gap_size = new_gap_size;
}

static void move_gap_to_index(Buffer* buffer, usize index) {
	const usize buffer_count = get_count(*buffer);
	assert(index < buffer_count);

	u32* index_ptr = &(*buffer)[index];

	if (index_ptr < buffer->gap) {
		const usize amount_to_move = buffer->gap - index_ptr;

		ch::mem_copy(buffer->gap + buffer->gap_size - amount_to_move, index_ptr, amount_to_move * sizeof(u32));

		buffer->gap = index_ptr;
	} else {
		const usize amount_to_move = index_ptr - (buffer->gap + buffer->gap_size);

		ch::mem_copy(buffer->gap, buffer->gap + buffer->gap_size, amount_to_move * sizeof(u32));

		buffer->gap += amount_to_move;
	}
}

u32* get_index_as_cursor(Buffer* buffer, usize index) {
	if (buffer->data + index <= buffer->gap) {
		return buffer->data + index;
	} 

	return buffer->data + buffer->gap_size + index;
}

void add_char(Buffer* buffer, u32 c, usize index) {
	if (buffer->gap_size <= 0) {
		buffer_resize(buffer, DEFAULT_GAP_SIZE);
	}

	if (get_count(*buffer) > 0) {
        move_gap_to_index(buffer, index);
    }

	u32* cursor = get_index_as_cursor(buffer, index);
	*cursor = c;
	buffer->gap += 1;
	buffer->gap_size -= 1;

    usize index_line = 0;
	usize index_column = 0;

	usize current_index = 0;
	for (usize i = 0; i < buffer->eol_table.count; i++) {
		usize line_size = buffer->eol_table[i];

		if (index < current_index + line_size) {
			index_column = index - current_index;
			break;
		}

		current_index += line_size;
		index_line += 1;
	}
	if (is_eol(c)) {
        const usize line_size = buffer->eol_table[index_line];
		buffer->eol_table[index_line] = index_column + 1;
		buffer->eol_table.insert(line_size - index_column, index_line + 1);
	} else if (index_line < buffer->eol_table.count) { // @Hack
        buffer->eol_table[index_line] += 1;
    }

    buffer->syntax_is_dirty = true;
}

void remove_at_index(Buffer* buffer, usize index) {
	const usize buffer_count = get_count(*buffer);
	assert(index < buffer_count);

	u32* cursor = get_index_as_cursor(buffer, index);
	if (cursor == buffer->data) {
		return;
	}

	move_gap_to_index(buffer, index);

	usize index_line = 0;
	usize current_index = 0;
	for (usize i = 0; i < buffer->eol_table.count; i++) {
		usize line_size = buffer->eol_table[i];

		if (index < current_index + line_size) {
			break;
		}

		current_index += line_size;
		index_line += 1;
	}

	buffer->eol_table[index_line] -= 1;
	
    const u32 c = (*buffer)[index - 1];
	if (is_eol(c)) {
		const usize amount_on_line = buffer->eol_table[index_line];
		buffer->eol_table.remove(index_line);
		buffer->eol_table[index_line - 1] += amount_on_line;
    }

	buffer->gap -= 1;
	buffer->gap_size += 1;

    buffer->syntax_is_dirty = true;
}

void remove_between(Buffer* buffer, usize first, usize last) {
    // @SLOW(Colby): This is slow as shit
    for (usize i = first; i < last; i++) {
        remove_at_index(buffer, i);
    }
}

usize get_count(const Buffer& buffer) {
	return buffer.allocated - buffer.gap_size;
}

usize get_line_index(const Buffer& buffer, usize index) {
	assert(index < buffer.eol_table.count);
	
	usize result = 0;
	for (usize i = 0; i < index; i++) {
		result += buffer.eol_table[i];
	}

	return result;
}
usize get_line_from_index(const Buffer& buffer, usize buffer_index) {
	assert(buffer_index < get_count(buffer));
	
    usize line = 0;

	usize index = 0;
	for (; line < buffer.eol_table.count; line++) {
        usize new_index = index + buffer.eol_table[line];
        if (new_index > buffer_index) {
            break;
        }
        index = new_index;
	}

	return line;
}

Buffer* get_buffer(const Buffer_View* view) {
	Buffer* result = editor_find_buffer(&g_editor, view->id);
	assert(result);
	return result;
}

void refresh_cursor_info(Buffer_View* view, bool update_desired /*=true*/) {
	Buffer* buffer = get_buffer(view);

    usize buffer_count = get_count(*buffer);
    if (view->cursor < 0) {
        view->cursor = 0;
    }
    if (view->cursor > buffer_count) {
        view->cursor = buffer_count;
    }

	view->current_line_number = get_line_from_index(*buffer, view->cursor);
	view->current_column_number = view->cursor - get_line_index(*buffer, view->current_line_number);


	if (update_desired) {
		view->desired_column_distance = get_column_distance(*view);
	}
}

void move_cursor_vertical(Buffer_View* view, s64 delta) {
	Buffer* buffer = get_buffer(view);

	if (delta == 0) return;

	const usize buffer_count = get_count(*buffer);
	if (buffer_count == 0) {
		return;
	}

	usize new_line = view->current_line_number + delta;
	if (delta < 0 && (s64)view->current_line_number <= -delta) {
		new_line = 0;
	}
	else if (new_line >= buffer->eol_table.count) {
		new_line = buffer->eol_table.count - 1;
	}

	const usize line_index = get_line_index(*buffer, new_line);

    usize desired_column_number = get_column_number_closest_to_distance(buffer, new_line, view->desired_column_distance);

	usize new_cursor_index = line_index;
	if (buffer->eol_table[new_line] <= desired_column_number) {
		new_cursor_index += buffer->eol_table[new_line] - 1;
	}
	else {
		new_cursor_index += desired_column_number;
	}

	view->cursor = new_cursor_index;
	refresh_cursor_info(view, false);
}

void seek_line_start(Buffer_View* view) {
	// @Todo: indent-sensitive home seeking
	view->cursor -= view->current_column_number;
	refresh_cursor_info(view);
}

void seek_line_end(Buffer_View* view) {
	Buffer* buffer = get_buffer(view);
	if (buffer->eol_table.count) {
		const usize line_length = buffer->eol_table[view->current_line_number];
		if (!line_length) {
			assert(view->current_column_number == 0);
		}
		else {
			view->cursor += line_length - view->current_column_number - 1;
		}
	}
	refresh_cursor_info(view);
}

void seek_horizontal(Buffer_View* view, bool right) {
	Buffer* buffer = get_buffer(view);
	const usize buffer_count = get_count(*buffer);

	if (right) {
		if (view->cursor == buffer_count) {
			return;
		}
		view->cursor += 1;
	}
	else {
		if (view->cursor == 0) {
			return;
		}
		view->cursor -= 1;
	}

	while (view->cursor > 0 && view->cursor < buffer_count) {
		const u32 c = (*buffer)[view->cursor];

		if (ch::is_whitespace(c) || ch::is_symbol(c)) break;

		if (right) {
			view->cursor += 1;
		}
		else {
			view->cursor -= 1;
		}
	}

	refresh_cursor_info(view);
}

usize pick_index(Buffer_View* view, Vector2 pos) {
	const Vector2 v_pos = get_view_position(*view);
    float x = v_pos.x;
    float y = v_pos.y;

    const float font_height = g_editor.loaded_font.size;

    Buffer* buffer = get_buffer(view);

    usize line_offset = (usize)((view->current_scroll_y + pos.y) / font_height);
    if (line_offset >= buffer->eol_table.count) {
        line_offset = buffer->eol_table.count - 1;
    }

    y += line_offset * font_height - view->current_scroll_y;

    if (line_offset >= buffer->eol_table.count) {
        line_offset = buffer->eol_table.count - 1;
    }

    const usize start_index = get_line_index(*buffer, line_offset);
    const usize buffer_count = get_count(*buffer);
    for (usize i = start_index; i < buffer_count; i++) {
        const u32 c = (*buffer)[i];

        const Font_Glyph* glyph = font_find_glyph(&g_editor.loaded_font, c);
        if (!glyph) glyph = font_find_glyph(&g_editor.loaded_font, '?');
        if (ch::is_whitespace(c)) {
            glyph = font_find_glyph(&g_editor.loaded_font, ' ');
        }

        float width = glyph->advance;
        if (c == '\t') {
            width *= 4.f;
        }

        const float x0 = x;
        const float y0 = y;

        const float x1 = x0 + width;
        const float y1 = y0 + font_height;

        if (pos.x >= x0 && pos.x <= x1 && pos.y >= y0 && pos.y <= y1) {
            return i;
        }
        x += width;

        if (is_eol(c)) {
            return i;
        }
    }

    return 0;
}

static bool has_valid_selection(const Buffer_View& view) {
	return view.cursor != view.selection;
}

static void remove_selection(Buffer_View* view) {
	assert(has_valid_selection(*view));

	Buffer* buffer = get_buffer(view);

	if (view->cursor > view->selection) {
		for (usize i = view->cursor; i > view->selection; i--) {
			remove_at_index(buffer, i);
		}
		view->cursor = view->selection;
	} else {
		for (usize i = view->selection; i > view->cursor; i--) {
			remove_at_index(buffer, i);
		}
		view->selection = view->cursor;
	}
}

static void ensure_cursor_in_view(Buffer_View* view) {
    const float font_height = g_editor.loaded_font.size;
    const float cursor_scroll_y = view->current_line_number * font_height;
	const float view_inner_height = get_view_inner_size(*view).y;

	if (cursor_scroll_y + font_height > view->target_scroll_y + view_inner_height) {
		const float new_scroll_y = cursor_scroll_y - view_inner_height + font_height;
		view->target_scroll_y = new_scroll_y;
		view->current_scroll_y = new_scroll_y;
	} else if (cursor_scroll_y < view->target_scroll_y) {
		const float new_scroll_y = cursor_scroll_y;
		view->target_scroll_y = new_scroll_y;
		view->current_scroll_y = new_scroll_y;
	}
}

static void add_char_from_view(Buffer_View* view, u32 c) {
	Buffer* buffer = get_buffer(view);

	if (has_valid_selection(*view)) {
		remove_selection(view);
	}

	add_char(buffer, c, view->cursor);
	view->cursor += 1;
	view->selection += 1;
	refresh_cursor_info(view);
}

static void char_entered(void* owner, Event* event) {
	Buffer_View* view = (Buffer_View*)owner;
	Buffer* buffer = get_buffer(view);


	add_char_from_view(view, event->c);
}

static void key_pressed(void* owner, Event* event) {
	Buffer_View* view = (Buffer_View*)owner;
	Editor_State* editor = &g_editor;
	Input_State* input = &editor->input_state;
	Buffer* buffer = get_buffer(view);
	const usize buffer_count = get_count(*buffer);

	const u32 key_code = event->key_code;

	switch (key_code) {
	case KEY_LEFT:
		if (view->cursor > 0) {
			if (input->ctrl_is_down) {
				seek_horizontal(view, false);
				if (!input->shift_is_down) {
					view->selection = view->cursor;
				}
				break;
			}
			view->cursor -= 1;
            if (!input->shift_is_down) {
                view->selection = view->cursor;
            }
			refresh_cursor_info(view);
		}
		break;
	case KEY_RIGHT:
		if (view->cursor < buffer_count - 1) {
			if (input->ctrl_is_down) {
				seek_horizontal(view, true);
				if (!input->shift_is_down) {
					view->selection = view->cursor;
				}
				break;
			}
			view->cursor += 1;
            if (!input->shift_is_down) {
                view->selection = view->cursor;
            }
			refresh_cursor_info(view);
		}
		break;
	case KEY_ENTER: {
		add_char_from_view(view, '\n');

        // @NOTE(Colby): Auto tabbing
        const usize prev_line = view->current_line_number - 1;
        assert(prev_line >= 0);
        const usize line_length = buffer->eol_table[prev_line];
        const usize prev_line_index = get_line_index(*buffer, view->current_line_number - 1);
        for (usize i = 0; i < line_length; i++) {
            const u32 c = (*buffer)[i + prev_line_index];
            if (!ch::is_whitespace(c) || c == ch::eol) {
                break;
            }
            add_char_from_view(view, c);
        }
    } break;
	case KEY_UP:
	case KEY_DOWN: {
		const s64 delta = key_code == KEY_UP ? -1 : 1;
		move_cursor_vertical(view, delta);
		if (!input->shift_is_down) {
			view->selection = view->cursor;
		}
	} break;
	case KEY_HOME:
	case KEY_END: {
        if (input->ctrl_is_down) {
            if (key_code == KEY_HOME) {
                view->cursor = 0;
            } else {
                view->cursor = get_count(*buffer) - 1;
            }
            refresh_cursor_info(view, true);
        } else {
		    if (key_code == KEY_HOME) {
		    	seek_line_start(view);
		    } else {
		    	seek_line_end(view);
		    }
        }
        if (!input->shift_is_down) {
            view->selection = view->cursor;
        }

	} break;
	case KEY_BACKSPACE:
		if (view->cursor > 0) {
			if (input->ctrl_is_down) {
				seek_horizontal(view, false);
			}

			if (has_valid_selection(*view)) {
				remove_selection(view);
                refresh_cursor_info(view);
			} else {
				remove_at_index(buffer, view->cursor);
				view->cursor -= 1;
				view->selection = view->cursor;
				refresh_cursor_info(view);
			}
		}
		break;
	case KEY_DELETE: {
		if (view->cursor < buffer_count - 1) {
			if (input->ctrl_is_down) {
				seek_horizontal(view, true);
			}

			if (has_valid_selection(*view)) {
				remove_selection(view);
                refresh_cursor_info(view);
			} else {
				remove_at_index(buffer, view->cursor + 1);
				refresh_cursor_info(view);
			}
		}
	} break;
	case KEY_PAGEUP:
	case KEY_PAGEDOWN: {
		const float font_height = editor->loaded_font.size;
		const usize lines_in_view = (usize)(get_view_inner_size(*view).y / font_height);
		const usize lines_scrolled = (usize)(view->current_scroll_y / font_height);


		if (input->ctrl_is_down) {
			move_cursor_vertical(view, key_code == KEY_PAGEUP ? lines_scrolled - view->current_line_number : (lines_in_view + lines_scrolled) - view->current_line_number);
		} else {
            usize lines_to_scroll = lines_in_view;
            if (lines_to_scroll > 3) lines_to_scroll -= 3;
			move_cursor_vertical(view, key_code == KEY_PAGEUP ? (usize)-(s64)lines_to_scroll : lines_to_scroll);
		}
		if (!input->shift_is_down) {
			view->selection = view->cursor;
		}

	} break;
    case KEY_F1: { // @Temporary @Debug
        extern bool use_dfa_parser;
        use_dfa_parser = !use_dfa_parser;
        buffer->syntax_is_dirty = true;
    } break;
    case KEY_F2: { // @Temporary @Debug
        extern bool debug_show_sections;
        debug_show_sections = !debug_show_sections;
    } break;
	case 'V':
		if (editor->input_state.ctrl_is_down) {
			ch::String results;
			if (os_copy_out_of_clipboard(&results)) {
				for (usize i = 0; i < results.count; i++) {
					tchar c = results[i];
					if (c == '\r') continue;
					add_char_from_view(view, c);
				}
				ch::free(results.data);
			}
		}
		break;
	case 'C':
		if (editor->input_state.ctrl_is_down && has_valid_selection(*view)) {
			const usize buffer_count = get_count(*buffer);
			const usize index = (view->cursor > view->selection) ? view->selection : view->cursor;
			const usize size = (view->cursor > view->selection) ? (view->cursor - view->selection) : (view->selection - view->cursor);
			tchar* out_buffer = (tchar*)ch::malloc(size + 1);
			defer(ch::free(out_buffer));
			for (usize i = 0; i < size; i++) {
				out_buffer[i] = (tchar)(*buffer)[index + i];
			}
			out_buffer[size] = 0;
			os_copy_to_clipboard(out_buffer, size + 1);
		}
		break;
    case 'X':
        if (editor->input_state.ctrl_is_down && has_valid_selection(*view)) {
            const usize buffer_count = get_count(*buffer);
            const usize index = (view->cursor > view->selection) ? view->selection : view->cursor;
            const usize size = (view->cursor > view->selection) ? (view->cursor - view->selection) : (view->selection - view->cursor);
            tchar* out_buffer = (tchar*)ch::malloc(size + 1);
			defer(ch::free(out_buffer));
			for (usize i = 0; i < size; i++) {
				out_buffer[i] = (tchar)(*buffer)[index + i];
			}
			out_buffer[size] = 0;
			os_copy_to_clipboard(out_buffer, size + 1);
            remove_selection(view);
        }
        break;
	}
	ensure_cursor_in_view(view);
}

void buffer_view_lost_focus(Buffer_View* view) {
	unbind_event_listener(&g_editor.input_state, make_event_listener(view, char_entered, ET_Char_Entered));
	unbind_event_listener(&g_editor.input_state, make_event_listener(view, key_pressed, ET_Key_Pressed));
}

void buffer_view_gained_focus(Buffer_View* view) {
	bind_event_listener(&g_editor.input_state, make_event_listener(view, char_entered, ET_Char_Entered));
	bind_event_listener(&g_editor.input_state, make_event_listener(view, key_pressed, ET_Key_Pressed));
}

Vector2 get_view_inner_size(const Buffer_View& view) {
	const float font_height = g_editor.loaded_font.size;
	const Vector2 padding = v2(font_height / 2.f);
	const float bar_height = font_height + padding.y;
	const float height = (float)os_window_height() - bar_height;
	const float width = (float)os_window_width() / g_editor.views_count;
	return v2(width, height);
}

Vector2 get_view_size(const Buffer_View& view) {
	return v2((float)os_window_width() / g_editor.views_count, (float)os_window_height());
}

Vector2 get_view_position(const Buffer_View& view) {
	const usize view_index = &view - &g_editor.views[0];
	const float width = get_view_size(view).x;
	return v2(width * view_index, 0.f);
}

float get_column_distance(const Buffer_View & view) {
    const Buffer* const buffer = get_buffer(&view);

    const usize n = view.current_column_number;

    const Font* font = &g_editor.loaded_font;
    const usize line_begin = get_line_index(*buffer, view.current_line_number);
    const usize line_end = line_begin + buffer->eol_table[view.current_line_number]; 

    const usize buffer_count = get_count(*buffer);

    float result = 0;

    const Font_Glyph* space_glyph = font_find_glyph(font, ' ');
    assert(space_glyph);

    for (usize i = 0; i < n; i++) {
        const usize index = line_begin + i;
        assert(index < buffer_count);
        if (index >= buffer_count || index >= line_end) {
            break;
        }
        const u32 c = (*buffer)[index];
        const Font_Glyph* glyph = font_find_glyph(font, c);
        if (!glyph) glyph = font_find_glyph(&g_editor.loaded_font, '?');
        if (ch::is_whitespace(c)) {
            glyph = space_glyph;
        }
        float advance = glyph->advance;
        if (c == '\t') {
            advance *= 4;
        }
        result += advance / font->size;
    }
    return result;
}
usize get_column_number_closest_to_distance(const Buffer* const buffer, usize line, float distance) {
    const Font* font = &g_editor.loaded_font;
    const usize line_begin = get_line_index(*buffer, line);
    const usize line_end = line_begin + buffer->eol_table[line];

    const usize buffer_count = get_count(*buffer);

    usize result = 0;

    const Font_Glyph* space_glyph = font_find_glyph(font, ' ');
    assert(space_glyph);

    for (float pointer = 0; pointer < distance;) {
        const usize index = line_begin + result;
        if (index >= buffer_count || index >= line_end) {
            break;
        }
        const u32 c = (*buffer)[index];
        const Font_Glyph* glyph = font_find_glyph(font, c);
        if (!glyph) glyph = font_find_glyph(&g_editor.loaded_font, '?');
        if (ch::is_whitespace(c)) {
            glyph = space_glyph;
        }
        float advance = glyph->advance;
        if (c == '\t') {
            advance *= 4;
        }
        pointer += advance / font->size;
        result++;
    }
    return result;
}
