#pragma once

#include <ch_stl/input.h>
#include <ch_stl/math.h>

extern ch::Vector2 last_mouse_position;
extern ch::Vector2 current_mouse_position;

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

bool is_mb_down(u8 mb);
bool did_mb_go_down(u8 mb);
bool did_mb_go_up(u8 mb);

bool is_key_down(u8 key);
bool did_key_go_down(u8 key);
bool did_key_go_up(u8 key);