#pragma once

#include "types.h"
#include "string.h"
#include "math.h"

enum OS_Cursor_Type {
	CT_Arrow,
	CT_VResize,
	CT_HResize,
};

void* os_get_window_handle();
void os_poll_window_events();

u32 os_window_width();
u32 os_window_height();

void os_set_path_to_fonts();
String os_get_path();
bool os_set_path(const String& string);

double os_get_time();

String os_load_file_into_memory(const char* path);

void os_set_cursor_type(OS_Cursor_Type type);

Vector2 os_get_mouse_position();

