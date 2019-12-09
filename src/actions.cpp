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

// @Temporary: we need all this to define the GetOpenFileNameW function.
// If you think this is "ugly", then I challenge you to include <Windows.h> in this file.
using WCHAR = wchar_t;
using LPWSTR = wchar_t*;
using LPCWSTR = const wchar_t*;
typedef UINT_PTR (__stdcall *LPOFNHOOKPROC) (HWND, UINT, WPARAM, LPARAM);
typedef struct tagOFNW {
   DWORD        lStructSize;
   HWND         hwndOwner;
   HINSTANCE    hInstance;
   LPCWSTR      lpstrFilter;
   LPWSTR       lpstrCustomFilter;
   DWORD        nMaxCustFilter;
   DWORD        nFilterIndex;
   LPWSTR       lpstrFile;
   DWORD        nMaxFile;
   LPWSTR       lpstrFileTitle;
   DWORD        nMaxFileTitle;
   LPCWSTR      lpstrInitialDir;
   LPCWSTR      lpstrTitle;
   DWORD        Flags;
   WORD         nFileOffset;
   WORD         nFileExtension;
   LPCWSTR      lpstrDefExt;
   LPARAM       lCustData;
   LPOFNHOOKPROC lpfnHook;
   LPCWSTR      lpTemplateName;
#ifdef _MAC
   LPEDITMENU   lpEditInfo;
   LPCSTR       lpstrPrompt;
#endif
#if (_WIN32_WINNT >= 0x0500)
   void *        pvReserved;
   DWORD        dwReserved;
   DWORD        FlagsEx;
#endif // (_WIN32_WINNT >= 0x0500)
} OPENFILENAMEW, *LPOPENFILENAMEW;
extern "C" {
__declspec(dllimport) BOOL __stdcall GetOpenFileNameW(OPENFILENAMEW*);
__declspec(dllimport) int __stdcall WideCharToMultiByte(unsigned int CodePage, unsigned long dwFlags,
                                                        WCHAR *lpWideCharStr, int cchWideChar,
                                                        char *lpMultiByteStr, int cbMultiByte,
                                                        const char *lpDefaultChar, int *lpUsedDefaultChar);
}

void open_dialog() {

    OPENFILENAMEW open_file_name = {};
    open_file_name.lStructSize = sizeof(OPENFILENAMEW);
    open_file_name.hwndOwner = nullptr;
    open_file_name.hInstance = nullptr;
    open_file_name.lpstrFilter = L"" "All Files\0" "*.*\0";
    open_file_name.lpstrCustomFilter = nullptr;
    open_file_name.nMaxCustFilter = 0;
    open_file_name.nFilterIndex = 0;
    
    enum { TEMP_BUF_COUNT = 65536 };
    wchar_t temp_buf[TEMP_BUF_COUNT];
    
    open_file_name.lpstrFile = temp_buf;
    open_file_name.nMaxFile = TEMP_BUF_COUNT;
    
    if (open_file_name.nMaxFile > 0) open_file_name.lpstrFile[0] = 0;

    open_file_name.lpstrFileTitle = nullptr;
    open_file_name.nMaxFileTitle = 0;
    
    open_file_name.lpstrInitialDir = L"C:\\";
    
    open_file_name.lpstrTitle = L"Open Image";
    open_file_name.Flags = 0;
    open_file_name.nFileOffset = 0;
    open_file_name.nFileExtension = 0;
    open_file_name.lpstrDefExt = L"";
    open_file_name.lCustData = 0;
    open_file_name.lpfnHook = nullptr;
    open_file_name.lpTemplateName = nullptr;
    
    auto result = GetOpenFileNameW(&open_file_name);
    
    if (!result) {
        // handle user cancel or error
        
        //auto err = CommDlgExtendedError();
        //printf("CommDlg error: %\n"_s, cast(int) err);
        //return {};
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

    ch::Path path = {};

    assert(filename16_len <= buffer->absolute_path.allocated);
    // CP_UTF8 == 65001
    WideCharToMultiByte(65001, 0, (LPWSTR)filename16, (int)filename16_len, (char *)path.data, path.allocated, nullptr, nullptr);
    path.count = filename16_len;
    
	buffer->load_file_into_buffer(path);

    return;
}
