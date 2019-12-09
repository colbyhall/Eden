#include "actions.h"

#include "buffer_view.h"
#include "buffer.h"

#include <ch_stl/string.h>

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

#if CH_PLATFORM_WINDOWS
typedef UINT_PTR (__stdcall *LPOFNHOOKPROC) (HWND, UINT, WPARAM, LPARAM);
struct OPENFILENAMEA {
	DWORD        lStructSize;
	HWND         hwndOwner;
	HINSTANCE    hInstance;
	LPCSTR       lpstrFilter;
	LPSTR        lpstrCustomFilter;
	DWORD        nMaxCustFilter;
	DWORD        nFilterIndex;
	LPSTR        lpstrFile;
	DWORD        nMaxFile;
	LPSTR        lpstrFileTitle;
	DWORD        nMaxFileTitle;
	LPCSTR       lpstrInitialDir;
	LPCSTR       lpstrTitle;
	DWORD        Flags;
	WORD         nFileOffset;
	WORD         nFileExtension;
	LPCSTR       lpstrDefExt;
	LPARAM       lCustData;
	LPOFNHOOKPROC lpfnHook;
	LPCSTR       lpTemplateName;
#ifdef _MAC
	LPEDITMENU   lpEditInfo;
	LPCSTR       lpstrPrompt;
#endif
#if (_WIN32_WINNT >= 0x0500)
	void* pvReserved;
	DWORD        dwReserved;
	DWORD        FlagsEx;
#endif // (_WIN32_WINNT >= 0x0500)
};
using LPOPENFILENAMEA = OPENFILENAMEA*;

extern "C" {
	DLL_IMPORT BOOL __stdcall GetOpenFileNameA(OPENFILENAMEA*);
}

void open_dialog() {

    OPENFILENAMEA open_file_name = {};
    open_file_name.lStructSize = sizeof(OPENFILENAMEA);
    open_file_name.hwndOwner = nullptr;
    open_file_name.hInstance = nullptr;
    open_file_name.lpstrFilter = "" "All Files\0" "*.*\0";
    open_file_name.lpstrCustomFilter = nullptr;
    open_file_name.nMaxCustFilter = 0;
    open_file_name.nFilterIndex = 0;
    
    constexpr usize TEMP_BUF_COUNT = 65536;
    char temp_buf[TEMP_BUF_COUNT];
    
    open_file_name.lpstrFile = temp_buf;
    open_file_name.nMaxFile = TEMP_BUF_COUNT;
    
    if (open_file_name.nMaxFile > 0) open_file_name.lpstrFile[0] = 0;

    open_file_name.lpstrFileTitle = nullptr;
    open_file_name.nMaxFileTitle = 0;
    
	const ch::Path current_path = ch::get_current_path();

    open_file_name.lpstrInitialDir = current_path;
    
    open_file_name.lpstrTitle = "Open File";
    open_file_name.lpstrDefExt = "";
    open_file_name.lCustData = 0;
    open_file_name.lpfnHook = nullptr;
    open_file_name.lpTemplateName = nullptr;
    
	if (!GetOpenFileNameA(&open_file_name)) {
		return;
	}
    
    u16 *filename16 = (u16*)open_file_name.lpstrFile;
    u64 filename16_len = 0;
    
    for (auto p = filename16; *p; p += 1) filename16_len += 1;
    
    Buffer_View* const view = get_focused_view();
    assert(view);
    Buffer_ID id = create_buffer();
    view->the_buffer = id;
    Buffer* const buffer = find_buffer(id);
    assert(buffer);

	ch::Path path(open_file_name.lpstrFile);
    
	buffer->load_file_into_buffer(path);

    return;
}
#else

void open_dialog() {}

#endif