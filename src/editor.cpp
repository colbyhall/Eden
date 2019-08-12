#include "editor.h"
#include "draw.h"
#include "buffer.h"
#include "gui.h"
#include "input.h"

#include <ch_stl/opengl.h>
#include <ch_stl/time.h>

ch::Window the_window;

const tchar* window_title = CH_TEXT("YEET");
Font font;

static void editor_tick(f32 dt) {

}

static void editor_draw() {
	draw_begin();

	if (gui_button(GUI_ID(&the_window), CH_TEXT("FUCK"), 0.f, 0.f, 200.f, 100.f)) {
		ch::std_out << "wow" << ch::eol;
	}

	draw_end();
}

#if CH_PLATFORM_WINDOWS && !CH_BUILD_DEBUG
int WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
#else
int main() {
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

	the_input_state.init();

	const bool draw_inited = draw_init();
	assert(draw_inited);

	// @TEMP(CHall): Load font and get size
	{
		ch::Path p = ch::get_os_font_path();
		p.append(CH_TEXT("consola.ttf"));
		const bool loaded_font = load_font_from_path(p, &font);
		assert(loaded_font);
		font.size = 16;
		font.pack_atlas();
	}

	the_window.set_visibility(true);

	f64 last_frame_time = ch::get_time_in_seconds();
	while (!the_input_state.exit_requested) {
		const f64 current_time = ch::get_time_in_seconds();
		const f32 dt = (f32)(current_time - last_frame_time);
		last_frame_time = current_time;

		the_input_state.process_input();
		editor_tick(dt);
		editor_draw();
	}
}