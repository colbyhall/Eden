#pragma once
#include "types.h"
#include "array.h"
#include "buffer.h"
#include "font.h"
#include "math.h"
#include "input.h"

#define WINDOW_TITLE "YEET"

struct Editor_State {
	bool is_running;

	Input_State input_state;

	Font loaded_font;

	float dt;

	Array<Buffer> loaded_buffers;
	Buffer_ID last_buffer_id;
};

void editor_init(Editor_State* editor);
void editor_loop(Editor_State* editor);
void editor_shutdown(Editor_State* editor);

void editor_poll_input(Editor_State* editor);
void editor_tick(Editor_State* editor, float dt);
void editor_draw(Editor_State* editor);

Buffer* editor_create_buffer(Editor_State* editor);
Buffer* editor_find_buffer(Editor_State* editor, Buffer_ID id);
bool editor_destroy_buffer(Editor_State* editor, Buffer_ID id);


