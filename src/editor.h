#pragma once

#include <ch_stl/window.h>
#include "buffer.h"

extern ch::Window the_window;
extern struct Font the_font;
extern Buffer_ID messages_buffer;

Buffer_ID create_buffer();
bool remove_buffer(Buffer_ID id);
Buffer* find_buffer(Buffer_ID id);

inline Buffer* get_messages_buffer() {
    return find_buffer(messages_buffer);
}

#define print_to_messages(fmt, ...) get_messages_buffer()->print_to(fmt, __VA_ARGS__)