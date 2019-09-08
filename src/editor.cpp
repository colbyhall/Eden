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

ch::Window the_window;

const tchar* window_title = CH_TEXT("YEET");
Font the_font;

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

static void tick_editor(f32 dt) {
	tick_views(dt);
}

#if CH_PLATFORM_WINDOWS
int WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
#else
int main() {
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
	}
	const bool is_gl_current = ch::make_current(the_window);
	assert(is_gl_current);

	if (config.was_maximized) the_window.maximize();

	the_window.on_resize = [](const ch::Window& window) {
		tick_editor(0.f);
		draw_editor();
	};

	init_draw();
	init_input();

	Buffer* buffer = create_buffer();
	push_view(buffer->id);

	// @TEMP(CHall): Load font and get size
	{
		ch::Path p = ch::get_os_font_path();
		p.append(CH_TEXT("consola.ttf"));
		const bool loaded_font = load_font_from_path(p, &the_font);
		assert(loaded_font);
		the_font.size = get_config().font_size;
		the_font.pack_atlas();
	}

	the_window.set_visibility(true);

	ch::Allocator temp_arena = ch::make_arena_allocator(1024 * 1024 * 32);
	ch::context_allocator = temp_arena;

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
	}

	shutdown_config();
}