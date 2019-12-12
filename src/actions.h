#pragma once

/** adds a newline to the current buffer. */
void newline();

/** removes the char at the cursor in the current buffer. */
void backspace();

void move_cursor_right(bool move_selection);

void move_cursor_left(bool move_selection);

void move_cursor_up(bool move_selection);

void move_cursor_down(bool move_selection);

void save_buffer();

void open_dialog();