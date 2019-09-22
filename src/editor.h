#pragma once

#include <ch_stl/window.h>
#include "buffer.h"

extern ch::Window the_window;
extern struct Font the_font;
extern Buffer* messages_buffer;

Buffer* create_buffer();
bool remove_buffer(Buffer_ID id);
Buffer* find_buffer(Buffer_ID id);

#define print_to_messages(fmt, ...) messages_buffer->print_to(fmt, __VA_ARGS__)