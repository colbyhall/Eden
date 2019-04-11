#include "editor.h"
#include "os.h"
#include "opengl.h"
#include "draw.h"
#include "string.h"

#include <assert.h>

void editor_init(Editor* editor) {
	assert(gl_init());
	init_renderer();

	editor->font = load_font("data\\fonts\\Consolas.ttf");

	editor->is_running = true;
}

void editor_loop(Editor* editor) {
	while (editor->is_running) {
		os_poll_window_events();

		editor_draw(editor);
	}
}

void editor_shutdown(Editor* editor) {

}

void editor_on_window_resized(Editor* editor, u32 old_width, u32 old_height) {
	glViewport(0, 0, os_window_width(), os_window_height());
	render_right_handed();
}

void editor_draw(Editor* editor) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	const float window_width = (float)os_window_width();
	const float window_height = (float)os_window_height();
	
	{
		const float x0 = 0.f;
		const float y0 = 0.f;
		const float x1 = window_width;
		const float y1 = window_height;

		draw_rect(x0, y0, x1, y1, vec4_color(0x1a212d));
	}

	{
		const float height = 20.f;

		const float x0 = 0.f;
		const float y0 = window_height - (height * 2.25f);
		const float x1 = window_width;
		const float y1 = y0 + height;

		draw_rect(x0, y0, x1, y1, vec4_color(0x273244));

		String str = make_string("Scratch Buffer");
		draw_string(&str, x0, y0, height - 2.f, &editor->font);
	}


	gl_swap_buffers();
}
