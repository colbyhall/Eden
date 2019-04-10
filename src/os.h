#pragma once

#include "types.h"

void* os_get_window_handle();
void os_poll_window_events();

u32 os_window_width();
u32 os_window_height();