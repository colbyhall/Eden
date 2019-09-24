#include "editor.h"
#include "draw.h"
#include "buffer.h"
#include "gui.h"
#include "input.h"
#include "buffer_view.h"
#include "config.h"

#include <ch_stl/opengl.h>
#include <ch_stl/time.h>
#include <ch_stl/hash_table.h>

#if CH_PLATFORM_WINDOWS
#include "win32/icon_win32.h"
#endif

ch::Window the_window;

const tchar* window_title = CH_TEXT("eden");
Font the_font;
Buffer* messages_buffer;

ch::Hash_Table<Buffer_ID, Buffer> buffers(ch::get_heap_allocator());

static Buffer_ID last_id = 0;

Buffer* create_buffer() {
	const Buffer_ID id = last_id++;
	const usize index = buffers.push(id, Buffer(id));
	return &buffers[index];
}

bool remove_buffer(Buffer_ID id) {
	return buffers.remove(id);
}

Buffer* find_buffer(Buffer_ID id) {
	return buffers.find(id);
}

            int num_vertices_total;
static void tick_editor(f32 dt) {
	tick_views(dt);
#if 1//BUILD_DEBUG
    {
		const ch::Vector2 viewport_size = the_window.get_viewport_size();
		Vertical_Layout debug_layout((f32)viewport_size.ux - 300.f, 0.f, (f32)get_config().font_size + 5.f);
		tchar temp[100];
		ch::sprintf(temp, CH_TEXT("FPS: %f"), 1.f / dt);
		gui_label(temp, ch::magenta, debug_layout.at_x, debug_layout.at_y);
		debug_layout.row();

		for (const ch::Scoped_Timer& it : ch::scoped_timer_manager.entries) {
			ch::sprintf(temp, CH_TEXT("%s: %f"), it.name, it.get_gap());
			gui_label(temp, ch::magenta, debug_layout.at_x, debug_layout.at_y);
			debug_layout.row();
		}
        {
            ch::sprintf(temp, CH_TEXT("%d vertices"), num_vertices_total);
            num_vertices_total = 0;
            gui_label(temp, ch::magenta, debug_layout.at_x, debug_layout.at_y);
            debug_layout.row();
        }
	}
#endif
}

#if CH_PLATFORM_WINDOWS
enum  PROCESS_DPI_AWARENESS {
	PROCESS_DPI_UNAWARE,
	PROCESS_SYSTEM_DPI_AWARE,
	PROCESS_PER_MONITOR_DPI_AWARE
};

using Set_Process_DPI_Awareness = HRESULT(*)(PROCESS_DPI_AWARENESS value);

extern "C"
{
	__declspec(dllexport) DWORD NvOptimusEnablement = 0x0;
	__declspec(dllexport) DWORD AmdPowerXpressRequestHighPerformance = 0x0;
}
#endif

#if CH_PLATFORM_WINDOWS
int WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
#else
int main() {
#endif

#if CH_PLATFORM_WINDOWS
	ch::Library shcore_lib;
	if (ch::load_library(CH_TEXT("shcore.dll"), &shcore_lib)) {
		defer(shcore_lib.free());
		Set_Process_DPI_Awareness SetProcessDpiAwareness = shcore_lib.get_function<Set_Process_DPI_Awareness>(CH_TEXT("SetProcessDpiAwareness"));

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
		const bool window_created = ch::create_gl_window(window_title, width, height, 0, &the_window);
		assert(window_created);
#if CH_PLATFORM_WINDOWS
		HICON the_icon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1));
		assert(the_icon);
		SendMessage((HWND)the_window.os_handle, WM_SETICON, ICON_BIG, (LPARAM)the_icon);
#endif
	}
	const bool is_gl_current = ch::make_current(the_window);
	assert(is_gl_current);

	if (config.was_maximized) the_window.maximize();

	the_window.on_sizing = [](const ch::Window& window) {
		tick_editor(0.f);
		draw_editor();
	};

	init_draw();
	init_input();

	messages_buffer = create_buffer();
	messages_buffer->disable_parse = true;
	messages_buffer->print_to(CH_TEXT("Hello Sailor!\nWelcome to Eden\n"));

	Buffer* buffer = create_buffer();
	push_view(buffer->id);
	//push_view(messages_buffer->id);


    // @Temporary: Load test file.
    // If you have a file in bin/test_files/test_stb.h, use this to auto-load the file.
    // My test file is upwards of 10 megabytes, so it's not checked into git.
    // In the future this will be superseded by a flie loading system; this is just
    // to test parsing speeds. -phillip 2019-09-17
	if (true)
    {
        ch::File_Data fd = {};
		const ch::Path path = CH_TEXT("../test_files/10mb_file.h");
        if (ch::load_file_into_memory(path, &fd)) {
			defer(fd.free());
			buffer->gap_buffer.resize(fd.size); // Pre-allocate.
			for (usize i = 0; i < fd.size; i++) {
				if (fd.data[i] == '\r') {
					continue;
				}
				buffer->gap_buffer.push(fd.data[i]);
			}
			buffer->refresh_eol_table();
			buffer->refresh_line_column_table();
		} else {
			print_to_messages(CH_TEXT("Failed to find file %s"), path);
		}
    }

	// @TEMP(CHall): Load font and get size
	{
		ch::Path p = ch::get_os_font_path();
		p.append(CH_TEXT("consola.ttf"));
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
		draw_editor();
		try_refresh_config();

		if (!the_window.has_focus()) {
#if CH_PLATFORM_WINDOWS
			Sleep(100);
#endif
		}
	}

	shutdown_config();
}