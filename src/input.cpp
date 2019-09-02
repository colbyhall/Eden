#include "input.h"
#include "editor.h"

bool exit_requested = false;

bool is_mouse_over;
bool was_mouse_over;

bool lmb_down;
bool lmb_was_down;

ch::Vector2 last_mouse_position;
ch::Vector2 current_mouse_position;

void init_input() {
	the_window.on_exit_requested = [](const ch::Window& window) {
		exit_requested = true;
	};

	the_window.on_mouse_button_down = [](const ch::Window& window, u8 mouse_button) {
		if (mouse_button == CH_MOUSE_LEFT) lmb_down = true;
	};


	the_window.on_mouse_button_up = [](const ch::Window& window, u8 mouse_button) {
		if (mouse_button == CH_MOUSE_LEFT) lmb_down = false;
	};
}

void process_input() {
	last_mouse_position = current_mouse_position;
	was_mouse_over = is_mouse_over;
	lmb_was_down = lmb_down;

	ch::poll_events();

	ch::Vector2 u32_mouse_pos;
	is_mouse_over = the_window.get_mouse_position(&u32_mouse_pos);
	current_mouse_position.x = (f32)u32_mouse_pos.ux;
	current_mouse_position.y = (f32)u32_mouse_pos.uy;
}
