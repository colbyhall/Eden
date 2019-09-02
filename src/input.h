#pragma once

#include <ch_stl/input.h>
#include <ch_stl/math.h>

extern bool exit_requested;

extern bool is_mouse_over;
extern bool was_mouse_over;

extern bool lmb_down;
extern bool lmb_was_down;

extern ch::Vector2 last_mouse_position;
extern ch::Vector2 current_mouse_position;

void init_input();
void process_input();