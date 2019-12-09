#include "editor.h"
#include "draw.h"
#include "buffer.h"
#include "gui.h"
#include "input.h"
#include "buffer_view.h"
#include "config.h"
#include "buffer.h"

#include <ch_stl/opengl.h>
#include <ch_stl/time.h>
#include <ch_stl/hash_table.h>

#if CH_PLATFORM_WINDOWS
#include "win32/icon_win32.h"
#endif

ch::Window the_window;

const wchar_t* window_title = L"eden"; // @Hack.
Font the_font;

int num_vertices_total;

#define DEBUG_PERF 0
#define DEBUG_UTF8_FILE 0
#define DEBUG_LARGE_FILE 0

void tick_editor(f32 dt) {
	frame_begin();

	tick_views(dt);
#if DEBUG_PERF
    {
		const ch::Vector2 viewport_size = the_window.get_viewport_size();
		Vertical_Layout debug_layout((f32)viewport_size.ux - 300.f, 0.f, (f32)get_config().font_size + 5.f);
		char temp[100];
		ch::sprintf(temp, "FPS: %f", 1.f / dt);
		gui_label(temp, ch::magenta, debug_layout.at_x, debug_layout.at_y);
		debug_layout.row();

		for (const ch::Scoped_Timer& it : ch::scoped_timer_manager.entries) {
			ch::sprintf(temp, "%s: %f", it.name, it.get_gap());
			gui_label(temp, ch::magenta, debug_layout.at_x, debug_layout.at_y);
			debug_layout.row();
		}
        {
            ch::sprintf(temp, "%d vertices", num_vertices_total);
            num_vertices_total = 0;
            gui_label(temp, ch::magenta, debug_layout.at_x, debug_layout.at_y);
            debug_layout.row();
        }
	}
#endif

	frame_end();
}

#if CH_PLATFORM_WINDOWS
enum  PROCESS_DPI_AWARENESS {
	PROCESS_DPI_UNAWARE,
	PROCESS_SYSTEM_DPI_AWARE,
	PROCESS_PER_MONITOR_DPI_AWARE
};

using Set_Process_DPI_Awareness = HRESULT(*)(PROCESS_DPI_AWARENESS value);

using HICON = HANDLE;
#define WM_SETICON 0x80
#define ICON_BIG 1

extern "C" {
	DLL_IMPORT HICON WINAPI LoadIconA(HINSTANCE, LPCSTR);
	DLL_IMPORT LRESULT WINAPI SendMessageA(HWND, UINT, WPARAM, LPARAM);

    using TIMERPROC = void(*)(HWND Arg1, UINT Arg2, UINT_PTR Arg3, DWORD Arg4);
    DLL_IMPORT UINT_PTR WINAPI SetTimer(HWND hWnd, UINT_PTR nIDEvent, UINT uElapse, TIMERPROC lpTimerFunc);
    DLL_IMPORT BOOL WINAPI KillTimer(HWND hWnd, UINT_PTR nIDEvent);
}
#endif

#if CH_PLATFORM_WINDOWS
int WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
#else
int main() {
#endif

#if CH_PLATFORM_WINDOWS
	ch::Library shcore_lib;
	if (ch::load_library("shcore.dll", &shcore_lib)) {
		defer(shcore_lib.free());
		Set_Process_DPI_Awareness SetProcessDpiAwareness = shcore_lib.get_function<Set_Process_DPI_Awareness>("SetProcessDpiAwareness");

		if (SetProcessDpiAwareness) {
			SetProcessDpiAwareness(PROCESS_SYSTEM_DPI_AWARE);
		}
	}
#endif

	init_config();
	const Config& config = get_config();

	const bool gl_loaded = ch::load_gl();
	assert(gl_loaded);
	{
		const u32 width = config.last_window_width;
		const u32 height = config.last_window_height;
		const bool window_created = ch::create_gl_window((char*)window_title, width, height, 0, &the_window); // @Hack: (char*)
		assert(window_created);
#if CH_PLATFORM_WINDOWS
		HICON the_icon = LoadIconA(GetModuleHandleA(NULL), MAKEINTRESOURCEA(IDI_ICON1));
		assert(the_icon);
		SendMessageA((HWND)the_window.os_handle, WM_SETICON, ICON_BIG, (LPARAM)the_icon);
#endif
	}
	const bool is_gl_current = ch::make_current(the_window);
	assert(is_gl_current);

	if (config.was_maximized) the_window.maximize();

    the_window.on_sizing = [](const ch::Window& window) {
        tick_editor(1.0f);
    };

	init_draw();
	init_input();

	Buffer_ID buffer = create_buffer();
	push_view(buffer);

#if DEBUG_UTF8_FILE || DEBUG_LARGE_FILE
    {
        Buffer* const b = find_buffer(buffer);
#if DEBUG_UTF8_FILE
		const ch::Path path = "../test_files/utf8_test_file.txt";
#else
		const ch::Path path = "../test_files/10mb_file.h";
#endif
		b->load_file_into_buffer(path);
#if DEBUG_UTF8_FILE
		b->disable_parse = true;
#endif
    }
#endif


	// @TEMP(CHall): Load font and get size
	{
		ch::Path p = ch::get_os_font_path();
		p.append("consola.ttf");
		const bool loaded_font = load_font_from_path(p, &the_font);
		assert(loaded_font);
		the_font.size = get_config().font_size;
		the_font.pack_atlas();
	}

	ch::Allocator temp_arena = ch::make_arena_allocator(1024 * 1024 * 32);
	ch::context_allocator = temp_arena;

	the_window.set_visibility(true);
	
    f64 last_frame_time = ch::get_time_in_seconds();
	while (!is_exit_requested()) {
		const f64 current_time = ch::get_time_in_seconds();
		const f32 dt = (f32)(current_time - last_frame_time);
		last_frame_time = current_time;
		ch::reset_arena_allocator(&temp_arena);

		process_input();
		tick_editor(dt);
		try_refresh_config();

        ch::get_time_in_seconds();

		if (!the_window.has_focus()) {
			ch::sleep(100);
		}
	}

	shutdown_config();
}