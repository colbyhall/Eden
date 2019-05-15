#include "../os.h"
#include "../editor.h"
#include "../parsing.h"
#include "../opengl.h"
#include "../memory.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlwapi.h>

#include <stdlib.h>

HWND window_handle;
u32 window_width, window_height;

extern "C"
{
	__declspec(dllexport) DWORD NvOptimusEnablement = 0x0;
	__declspec(dllexport) DWORD AmdPowerXpressRequestHighPerformance = 0x0;
}

void* os_get_window_handle() {
	return window_handle;
}

void os_poll_window_events() {
	MSG msg;
	while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

u32 os_window_width() {
	return ::window_width;
}

u32 os_window_height() {
	return ::window_height;
}

typedef enum PROCESS_DPI_AWARENESS {
	PROCESS_DPI_UNAWARE,
	PROCESS_SYSTEM_DPI_AWARE,
	PROCESS_PER_MONITOR_DPI_AWARE
} PROCESS_DPI_AWARENESS;

typedef HRESULT(*Set_Process_DPI_Awareness)(PROCESS_DPI_AWARENESS value);

u16 surrogate_pair_first; // @Refactor: get some persistent state instead of global :)
static LRESULT window_proc(HWND handle, UINT message, WPARAM w_param, LPARAM l_param) {

	Editor_State* editor = (Editor_State*)GetWindowLongPtr(window_handle, GWLP_USERDATA);

	Event event_to_send;
	bool send_event = false;

	switch (message) {
		case WM_SIZE: {
			RECT rect;
			GetClientRect(handle, &rect);

			const u32 old_width = window_width;
			const u32 old_height = window_height;

			window_width = rect.right - rect.left;
			window_height = rect.bottom - rect.top;

			event_to_send = make_window_resize_event(old_width, old_height);
			send_event = true;
		} break;

		case WM_DESTROY: {
			event_to_send = make_exit_requested_event();
			send_event = true;
		} break;
		case WM_SIZING: {
			// @HACK(Colby): can be fixed by a sizing event
			editor_draw(editor);
		} break;

		case WM_MOUSEWHEEL: {
			const float delta = GET_WHEEL_DELTA_WPARAM(w_param);
			event_to_send = make_mouse_wheel_scrolled_event(delta);
			send_event = true;
		} break;

		case WM_CHAR: {
			u32 ch = (u32)w_param;
			if (ch < 32 && ch != '\t') break;
            if (ch == 127) break;

            if (ch >= 0xD800 && ch <= 0xDBFF) {
                surrogate_pair_first = ch;
                break;

            } else if (ch >= 0xDC00 && ch <= 0xDFFF) {
                u32 surrogate_pair_second = ch;
                ch = 0x10000;
                ch += (surrogate_pair_first & 0x03FF) << 10;
                ch += (surrogate_pair_second & 0x03FF);
            }
            event_to_send = make_char_entered_event(ch);
			send_event = true;
		} break;

		case WM_KEYDOWN: {
			const u8 key_code = (u8)w_param;
			event_to_send = make_key_pressed_event(key_code);
			send_event = true;
		} break;

		case WM_KEYUP: {
			const u8 key_code = (u8)w_param;
			// if (key_code < 32 || key_code > 126) break;
			event_to_send = make_key_released_event(key_code);
			send_event = true;
		} break;

		case WM_LBUTTONDOWN: {
			event_to_send = make_mouse_down_event(os_get_mouse_position());
			send_event = true;
		} break;

		case WM_LBUTTONUP: {
			event_to_send = make_mouse_up_event(os_get_mouse_position());
			send_event = true;
		} break;

		case WM_NCLBUTTONUP: 
		case WM_KILLFOCUS: {
			event_to_send = make_mouse_up_event(v2(0.f));
			send_event = true;
		} break;
	}

	if (send_event) {
		process_input_event(&editor->input_state, &event_to_send);
	}

	return DefWindowProc(handle, message, w_param, l_param);
}

double invfreq;

int WINAPI WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR cmd_line, int cmd_show) {
    {
        LARGE_INTEGER f;
        QueryPerformanceFrequency(&f);
        invfreq = 1.0 / f.QuadPart;
    }
	HMODULE dll_handle = LoadLibrary(TEXT("Shcore.dll"));
	if (dll_handle) {
		Set_Process_DPI_Awareness SetProcessDpiAwareness = (Set_Process_DPI_Awareness)GetProcAddress(dll_handle, "SetProcessDpiAwareness");

		if (SetProcessDpiAwareness) {
			SetProcessDpiAwareness(PROCESS_SYSTEM_DPI_AWARE);
		}
		FreeLibrary(dll_handle);
	}

	WNDCLASSEX window_class;
	memset(&window_class, 0, sizeof(window_class));
	window_class.cbSize = sizeof(WNDCLASSEX);
	window_class.lpfnWndProc = window_proc;
	window_class.hInstance = instance;
	window_class.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	window_class.lpszClassName = TEXT("class");
	window_class.hCursor = LoadCursor(NULL, IDC_ARROW);


	if (!RegisterClassEx(&window_class)) {
		return 1;
	}

	const u32 monitor_width = GetSystemMetrics(SM_CXSCREEN);
	const u32 monitor_height = GetSystemMetrics(SM_CYSCREEN);

	const u32 window_width = monitor_width - 640;
	const u32 window_height = (u32)(window_width / (16.f / 9.f));

	const u32 pos_x = monitor_width / 2 - window_width / 2;
	const u32 pos_y = monitor_height / 2 - window_height / 2;

	window_handle = CreateWindow(window_class.lpszClassName, TEXT(WINDOW_TITLE), WS_OVERLAPPEDWINDOW, pos_x, pos_y, window_width, window_height, NULL, NULL, instance, NULL);

	SetWindowLongPtr(window_handle, GWLP_USERDATA, (LONG_PTR)&g_editor);

	editor_init(&g_editor);
	ShowWindow(window_handle, SW_SHOW);
#if BUILD_DEBUG
	wglSwapIntervalEXT(false);
#else
	wglSwapIntervalEXT(true);
#endif
	editor_loop(&g_editor);
	editor_shutdown(&g_editor);

	return 0;
}

double os_get_time() {
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
	return t.QuadPart * invfreq;
}

void os_set_cursor_type(OS_Cursor_Type type) {
	HCURSOR new_cursor = NULL;
	switch (type) {
	case CT_Arrow:
		new_cursor = LoadCursor(NULL, IDC_ARROW);
		break;
	case CT_HResize:
		new_cursor = LoadCursor(NULL, IDC_SIZENS);
		break;
	case CT_VResize:
		new_cursor = LoadCursor(NULL, IDC_SIZEWE);
		break;
	}

	SetCursor(new_cursor);
}

Vector2 os_get_mouse_position() {
	POINT p;
	if (GetCursorPos(&p)) {
		if (ScreenToClient((HWND)os_get_window_handle(), &p)) {
			return {(float) p.x, (float)p.y };
		}
	}

	return v2(0.f);
}

void *os_virtual_reserve(size_t reserved_size) {
    return VirtualAlloc(nullptr, reserved_size, MEM_RESERVE, 0);
}
void os_virtual_commit(void *reserved_range, size_t committed_size) {
    VirtualAlloc(reserved_range, committed_size, MEM_RESERVE | MEM_COMMIT, 0);
}
void os_virtual_release(void *reserved_range) {
    VirtualFree(reserved_range, 0, MEM_RELEASE);
}

void os_set_path_to_fonts() {
	char buffer[MAX_PATH];
	GetWindowsDirectory(buffer, MAX_PATH);
	SetCurrentDirectory(buffer);
	SetCurrentDirectory(TEXT("Fonts"));

	GetCurrentDirectory(MAX_PATH, buffer);
}

String os_get_path() {
	u8* buffer = (u8*)c_alloc(MAX_PATH);
	GetCurrentDirectory(MAX_PATH, (char*)buffer);
	
	String result;
	result.data = buffer;
	result.allocated = MAX_PATH;
	result.count = strlen((const char*)buffer);

	return result;
}

bool os_set_path(const String& string) {
	return SetCurrentDirectory(string);
}