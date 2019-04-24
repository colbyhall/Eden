#include "../os.h"
#include "../editor.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlwapi.h>

#include <stdlib.h>

HWND window_handle;
u32 window_width, window_height;

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
	return window_width;
}

u32 os_window_height() {
	return window_height;
}

typedef enum PROCESS_DPI_AWARENESS {
	PROCESS_DPI_UNAWARE,
	PROCESS_SYSTEM_DPI_AWARE,
	PROCESS_PER_MONITOR_DPI_AWARE
} PROCESS_DPI_AWARENESS;

typedef HRESULT(*Set_Process_DPI_Awareness)(PROCESS_DPI_AWARENESS value);

static LRESULT window_proc(HWND handle, UINT message, WPARAM w_param, LPARAM l_param) {
	
	switch (message) {
	case WM_SIZE: {
		RECT rect;
		GetClientRect(handle, &rect);

		const u32 old_width = window_width;
		const u32 old_height = window_height;

		window_width = rect.right - rect.left;
		window_height = rect.bottom - rect.top;

		editor_on_window_resized(old_width, old_height);
		// game_state->on_window_resized(old_width, old_height);
	} break;

	case WM_DESTROY: {
		// game_state->on_exit_requested();
		is_running = false;
	} break;
	case WM_SIZING: {
		editor_draw();
	} break;

	case WM_MOUSEWHEEL: {
		float delta = GET_WHEEL_DELTA_WPARAM(w_param);
		editor_on_mousewheel_scrolled(delta);
	} break;

	case WM_CHAR: {
		u8 key = w_param;
		editor_on_key_pressed(key);
	} break;

	}

	return DefWindowProc(handle, message, w_param, l_param);
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR cmd_line, int cmd_show) {
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

#if 0
	{
		// @NOTE(Colby): This sucks really bad. I just wanted to set my working direction to parent directory of the exe's directory
		char exe_path[MAX_PATH];
		GetModuleFileName(NULL, exe_path, MAX_PATH);
		// HACK(Colby): This is deprecated but oh well
		PathRemoveFileSpec(exe_path);
		SetCurrentDirectory(exe_path);
		SetCurrentDirectory(TEXT(".."));
	}
#endif

	editor_init();
	ShowWindow(window_handle, SW_SHOW);
	wglSwapIntervalEXT(0);
	editor_loop();
	editor_shutdown();

	return 0;
}

String os_load_file_into_memory(const char* path) {
	HANDLE file_handle = CreateFile(path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	String result;

	// NOTE(Colby): Should we throw an assert here?
	if (file_handle == INVALID_HANDLE_VALUE) {
		result.length = 0;
		result.allocated = 0;
		result.data = NULL;
	}

	SetFilePointer(file_handle, 0, NULL, FILE_BEGIN);

	const DWORD file_size = GetFileSize(file_handle, NULL);

	u8* buffer = (u8*)malloc(file_size + 1);

	ReadFile(file_handle, buffer, file_size, 0, NULL);
	buffer[file_size] = 0;

	CloseHandle(file_handle);

	result.data = buffer;
	result.length = file_size;
	result.allocated = file_size + 1;

	return result;
}

u64 os_get_ms_time() {
	return (u64)GetTickCount64();
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