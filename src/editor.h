#pragma once
#include "types.h"

typedef struct {
	bool is_running;
} Editor;

void editor_init(Editor* editor);
void editor_loop(Editor* editor);
void editor_shutdown(Editor* editor);

void editor_on_window_resized(Editor* editor, u32 old_width, u32 old_height);

void editor_draw(Editor* editor);
