#pragma once

#include <ch_stl/ch_string.h>
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
ch::String os_get_path();
bool os_set_path(const ch::String& string);


double os_get_time();

ch::String os_load_file_into_memory(const char* path);

void os_set_cursor_type(OS_Cursor_Type type);

bool os_copy_to_clipboard(const void* buffer, usize size);
bool os_copy_out_of_clipboard(ch::String* out_string);

Vector2 os_get_mouse_position();

void *os_virtual_reserve(usize reserved_size);
void os_virtual_commit(void *reserved_range, usize committed_size);
void os_virtual_release(void *reserved_range);
