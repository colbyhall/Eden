#include "input.h"
#include "editor.h"

Input_State the_input_state;

void Input_State::init() {
	the_window.on_exit_requested = [](const ch::Window& window) {
		the_input_state.exit_requested = true;
	};

	the_window.on_mouse_button_down = [](const ch::Window& window, u8 mouse_button) {
		if (mouse_button == CH_MOUSE_LEFT) the_input_state.lmb_down = true;
	};


	the_window.on_mouse_button_up = [](const ch::Window& window, u8 mouse_button) {
		if (mouse_button == CH_MOUSE_LEFT) the_input_state.lmb_down = false;
	};
}

void Input_State::process_input() {
	last_mouse_position = current_mouse_position;
	was_mouse_over = is_mouse_over;
	lmb_was_down = lmb_down;
	ch::poll_events();
	ch::Vector2 u32_mouse_pos;
	is_mouse_over = the_window.get_mouse_position(&u32_mouse_pos);
	current_mouse_position.x = (f32)u32_mouse_pos.ux;
	current_mouse_position.y = (f32)u32_mouse_pos.uy;
}
