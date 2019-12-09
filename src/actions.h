#pragma once

/** adds a newline to the current buffer. */
void newline();

/** removes the char at the cursor in the current buffer. */
void backspace();

void move_cursor_right();

void move_cursor_left();

void move_cursor_up();

void move_cursor_down();

void save_buffer();

void open_dialog();