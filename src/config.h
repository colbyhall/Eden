#pragma once

#include <ch_stl/math.h>

#define CONFIG_VAR(macro) \
macro(u16, font_size, 24) \
macro(ch::Color, background_color, 0x052329FF) \
macro(ch::Color, foreground_color, 0xD6B58DFF) \
macro(ch::Color, cursor_color, 0x81E38EFF) \
macro(ch::Color, selection_color, 0x000EFFFF) \
macro(ch::Color, selected_text_color, ch::white) \
macro(bool, show_line_numbers, true) \
macro(ch::Color, line_number_background_color, 0x041E24FF) \
macro(ch::Color, line_number_text_color, 0x083945FF) \
macro(f32, scroll_speed, 5.f) \
macro(u16, tab_width, 4) \
macro(u32, last_window_width, 1920) \
macro(u32, last_window_height, 1080) \
macro(bool, was_maximized, false)

struct Config {
#define PUSH_VARS(t, n, v) t n = v;
CONFIG_VAR(PUSH_VARS);
#undef PUSH_VARS
};

extern Config default_config;

const Config& get_config();

void init_config();
void try_refresh_config();
void shutdown_config();

// @NOTE(Chall): this kind of sucks
void on_window_resize_config();
void on_window_maximized_config();