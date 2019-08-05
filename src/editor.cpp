#include "editor.h"
#include "draw.h"

#include <ch_stl/opengl.h>
#include <ch_stl/time.h>

ch::Window the_window;

static bool exit_requested = false;

const tchar* window_title = CH_TEXT("YEET");

static void editor_tick(f32 dt) {

}

static void editor_draw() {
	draw_begin();

	draw_quad(0.f, 0.f, 100.f, 100.f, ch::magenta);

	draw_end();
}

#if CH_PLATFORM_WINDOWS
int WinMain(HINSTANCE, HINSTANCE, LPTSTR, int) {
#endif
	const bool gl_loaded = ch::load_gl();
	assert(gl_loaded);
	{
		const u32 width = 1280;
		const u32 height = (u32)((f32)width * (9.f / 16.f));
		const bool window_created = ch::create_gl_window(window_title, width, height, 0, &the_window);
		assert(window_created);
	}
	const bool is_gl_current = ch::make_current(the_window);
	assert(is_gl_current);

	the_window.on_exit_requested = [](const ch::Window& window) {
		exit_requested = true;
	};

	the_window.on_resize = [](const ch::Window& window) {
		editor_draw();
	};

	const bool draw_inited = draw_init();
	assert(draw_inited);

	the_window.set_visibility(true);

	f64 last_frame_time = ch::get_time_in_seconds();
	while (!exit_requested) {
		const f64 current_time = ch::get_time_in_seconds();
		const f32 dt = (f32)(current_time - last_frame_time);
		last_frame_time = current_time;

		ch::poll_events();

		editor_tick(dt);
		editor_draw();
	}
}