#pragma once

#include <ch_stl/math.h>

struct Config {
	u16 font_size = 24;
	ch::Color background_color = 0x052329FF;
	ch::Color foreground_color = 0xD6B58DFF;
	ch::Color cursor_color = 0x81E38EFF;
	ch::Color selection_color = 0x000EFFFF;
	ch::Color selected_text_color = ch::white;
	bool show_line_numbers = false;
	ch::Color line_number_background_color = 0x041E24FF;
	ch::Color line_number_text_color = 0x083945FF;
	f32 scroll_speed = 5.f;
	u8 tab_width = 4;
};

extern Config default_config;

Config get_config();