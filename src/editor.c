#include "editor.h"
#include "os.h"
#include "opengl.h"
#include "draw.h"

#include <assert.h>

void editor_init(Editor* editor) {
	assert(gl_init());
	init_renderer();

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
	glClearColor(0.f, 0.f, 0.f, 1.f);

	draw_rect(100.f, 100.f, 200.f, 200.f, vec4_color(0xFF0000));

	gl_swap_buffers();
}
