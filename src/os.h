#pragma once

#include "types.h"
#include "string.h"
#include "math.h"

enum OS_Cursor_Type {
	CT_Arrow,
	CT_VResize,
	CT_HResize,
};

namespace OS {
	void* get_window_handle();
	void poll_window_events();

	u32 window_width();
	u32 window_height();

	u64 get_ms_time();

	String load_file_into_memory(const char* path);

	void set_cursor_type(OS_Cursor_Type type);

	Vector2 get_mouse_position();
}
