#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdlib.h>

#include "../types.h"

HWND window_handle;

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

typedef enum PROCESS_DPI_AWARENESS {
	PROCESS_DPI_UNAWARE,
	PROCESS_SYSTEM_DPI_AWARE,
	PROCESS_PER_MONITOR_DPI_AWARE
} PROCESS_DPI_AWARENESS;

typedef HRESULT(*Set_Process_DPI_Awareness)(PROCESS_DPI_AWARENESS value);

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
	window_class.lpfnWndProc = DefWindowProc;
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

	window_handle = CreateWindow(window_class.lpszClassName, TEXT("text_editor"), WS_OVERLAPPEDWINDOW, pos_x, pos_y, window_width, window_height, NULL, NULL, instance, NULL);
	ShowWindow(window_handle, SW_SHOW);
	bool is_running = true;
	while (is_running) {
		os_poll_window_events();
	}

	return 0;
}