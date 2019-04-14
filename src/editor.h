#pragma once
#include "types.h"
#include "font.h"

#define WINDOW_TITLE "YEET"

extern Font font;

typedef struct {
	bool is_running;
} Editor;

void editor_init(Editor* editor);
void editor_loop(Editor* editor);
void editor_shutdown(Editor* editor);

void editor_on_window_resized(Editor* editor, u32 old_width, u32 old_height);

void editor_draw(Editor* editor);
