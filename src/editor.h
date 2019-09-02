#pragma once

#include <ch_stl/window.h>
#include "buffer.h"

extern ch::Window the_window;
extern struct Font the_font;

Buffer* create_buffer();
bool remove_buffer(Buffer_ID id);
Buffer* find_buffer(Buffer_ID id);