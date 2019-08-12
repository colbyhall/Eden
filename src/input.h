#pragma once

#include <ch_stl/input.h>
#include <ch_stl/math.h>

struct Input_State {
	bool exit_requested = false;

	bool is_mouse_over;
	bool was_mouse_over;

	bool lmb_down;
	bool lmb_was_down;

	ch::Vector2 last_mouse_position;
	ch::Vector2 current_mouse_position;

	void init();
	void process_input();
};

extern Input_State the_input_state;