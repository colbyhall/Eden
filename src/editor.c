#include "editor.h"
#include "os.h"
#include "opengl.h"

#include <assert.h>

void editor_init(Editor* editor) {
	assert(gl_init());

	editor->is_running = true;
}

void editor_loop(Editor* editor) {
	while (editor->is_running) {
		os_poll_window_events();

		glClear(GL_COLOR_BUFFER_BIT);
		glClearColor(0.f, 1.f, 0.f, 1.f);

		gl_swap_buffers();
	}
}

void editor_shutdown(Editor* editor) {

}

void editor_draw(Editor* editor) {

}
