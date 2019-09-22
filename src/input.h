#pragma once

#include <ch_stl/input.h>
#include <ch_stl/math.h>

extern ch::Vector2 last_mouse_position;
extern ch::Vector2 current_mouse_position;
extern f32 current_mouse_scroll_y;

struct Key_Bind {
	bool shift_down;
	bool alt_down;
	bool ctrl_down;
	u32 key;
};

struct Action_Bind {
	const tchar* name;
	ch::Array<Key_Bind> keys;
};

void init_input();
void process_input();

bool is_exit_requested();

/**
 * Input Verbiage
 * 
 * down = mouse button or key is currently being held down
 * up = opposite of down
 * pressed = mouse button or key went down that frame
 * released = mouse button or key went up that frame
 */

bool is_mouse_button_down(u8 mb);
bool was_mouse_button_pressed(u8 mb);
bool was_mouse_button_released(u8 mb);

bool is_key_down(u8 key);
bool was_key_pressed(u8 key);
bool was_key_released(u8 key);