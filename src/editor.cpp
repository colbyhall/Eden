#include "editor.h"

#include "os.h"
#include "buffer.h"
#include "math.h"
#include "opengl.h"
#include "draw.h"
#include "memory.h"
#include "string.h"
#include "parsing.h"
#include "font.h"
#include "keys.h"

#include <assert.h>
#include <stdio.h>

static void editor_key_pressed(void* owner, Event* event) {
	Editor_State* editor = (Editor_State*)owner;

	Buffer* buffer = &editor->loaded_buffers[0];
	if (!buffer) {
		return;
	}
    bool edited = false;
	switch (event->key_code) {
	case KEY_ENTER:
		buffer_add_char(buffer, '\n');
        edited = true;
		break;
	case KEY_LEFT:
		buffer_move_cursor_horizontal(buffer, -1);
		break;
	case KEY_RIGHT:
		buffer_move_cursor_horizontal(buffer, 1);
		break;
	case KEY_UP:
		buffer_move_cursor_vertical(buffer, -1);
		break;
	case KEY_DOWN:
		buffer_move_cursor_vertical(buffer, 1);
		break;
	case KEY_HOME:
		buffer_seek_line_begin(buffer);
		break;
	case KEY_END:
		buffer_seek_line_end(buffer);
		break;
	case KEY_BACKSPACE:
		buffer_remove_before_cursor(buffer);
        edited = true;
		break;
	case KEY_DELETE:
		buffer_remove_at_cursor(buffer);
        edited = true;
		break;
	}
	// editor_bring_view_to_cursor(current_view, buffer);
	if (edited) {
        parse_syntax(&buffer->syntax, buffer, "c");
    }
}

static void editor_char_entered(void* owner, Event* event) {
	Editor_State* editor = (Editor_State*)owner;

	Buffer* buffer = &editor->loaded_buffers[0];
	if (!buffer) {
		return;
	}
	buffer_add_char(buffer, event->c);

	// editor_bring_view_to_cursor(current_view, buffer);
	parse_syntax(&buffer->syntax, buffer, "c");

}

static void editor_exit_requested(void* owner, Event* event) {
	Editor_State* editor = (Editor_State*)owner;
	editor->is_running = false;
}

void editor_init(Editor_State* editor) {
	// @NOTE(Colby): init systems here
	gl_init();
	draw_init();

	array_reserve(&editor->input_state.event_listeners, 1024);

	Event_Listener key_pressed = make_event_listener(editor, editor_key_pressed, ET_Key_Pressed);
	bind_event_listener(&editor->input_state, &key_pressed);

	Event_Listener char_entered = make_event_listener(editor, editor_char_entered, ET_Char_Entered);
	bind_event_listener(&editor->input_state, &char_entered);

	Event_Listener exit_requested = make_event_listener(editor, editor_exit_requested, ET_Exit_Requested);
	bind_event_listener(&editor->input_state, &exit_requested);

	editor->loaded_font = font_load_from_os("consola.ttf");
	
	Buffer* buffer = editor_create_buffer(editor);
	buffer_load_from_file(buffer, "C:/burnival/first.c");


	editor->is_running = true;
}

void editor_loop(Editor_State* editor) {
	u64 last_frame_time = os_get_ms_time();
	while (editor->is_running) {
		u64 current_time = os_get_ms_time();
		editor->dt = (current_time - last_frame_time) / 1000.f;
		last_frame_time = current_time;

		editor_poll_input(editor);
		editor_tick(editor, editor->dt);
		editor_draw(editor);
	}
}

void editor_shutdown(Editor_State* editor) {
}

void editor_poll_input(Editor_State* editor) {
	Input_State* input = &editor->input_state;
	input->last_mouse_position = input->current_mouse_position;
	os_poll_window_events();
	input->current_mouse_position = os_get_mouse_position();
}

void editor_tick(Editor_State* editor, float dt) {
}

void editor_draw(Editor_State* editor) {
	render_frame_begin();

	const float window_width = (float)os_window_width();
	const float window_height = (float)os_window_height();

	// @NOTE(Colby): Background
	{
		const float x0 = 0.f;
		const float y0 = 0.f;
		const float x1 = window_width;
		const float y1 = window_height;

		draw_rect(x0, y0, x1, y1, 0x052329);
	}

	// @NOTE(Colby): Command Bar and buffer drawing
	{
		const float font_height = FONT_SIZE;
		const Vector2 padding = v2(font_height / 2.f);
		const float bar_height = font_height + padding.y;
		{
			const float x0 = 0.f;
			const float y0 = 0.f;
			const float x1 = x0 + window_width;
			const float y1 = y0 + window_height - bar_height;
			draw_buffer_view(&editor->loaded_buffers[0], x0, y0, x1, y1, editor->loaded_font);
		}
		
		{
			immediate_begin();
			const float x0 = 0.f;
			const float y0 = window_height - bar_height;
			const float x1 = x0 + window_width;
			const float y1 = y0 + bar_height;
			immediate_quad(x0, y0, x1, y1, 0x052329);

			const float x = x0 + padding.x / 2.f;
			const float y = y0 + padding.y / 2.f;
			immediate_string("esc", x, y, 0xd6b58d, editor->loaded_font);
			immediate_flush();
		}
	}

#if BUILD_DEBUG
	{
		char buffer[256];
		sprintf_s(
			buffer, 256, "Draw Calls: %i\nVerts Drawn: %llu\nVerts Culled: %llu\nDT: %.6f\nAllocations: %llu\nAllocated: %f KB",
			draw_calls, verts_drawn, verts_culled, editor->dt, memory_num_allocations(), memory_amount_allocated() / 1024.f
		);
		Vector2 string_size = get_draw_string_size(buffer, editor->loaded_font);
		Vector2 padding = v2(10.f);
		float x = os_window_width() - (string_size.x + padding.x);
		float y = 0.f;

		immediate_begin();
		{

			float x0 = x;
			float y0 = y;
			float x1 = x0 + string_size.x + padding.x;
			float y1 = y0 + string_size.y + padding.y;

			immediate_quad(x0, y0, x1, y1, 0xAAAAAA);
		}

		{
			float x0 = x + padding.x / 2.f;
			float y0 = y + padding.y / 2.f;
			immediate_string(buffer, x0, y0, 0xFFFFFF, editor->loaded_font);
		}
		immediate_flush();
	}
#endif

	render_frame_end();
}

Buffer* editor_create_buffer(Editor_State* editor) {
	Buffer new_buffer = make_buffer(editor->last_buffer_id);
	editor->last_buffer_id += 1;
	size_t index = array_add(&editor->loaded_buffers, new_buffer);
	return &editor->loaded_buffers[index];
}

Buffer* editor_find_buffer(Editor_State* editor, Buffer_ID id) {
	for (Buffer& buffer : editor->loaded_buffers) {
		if (buffer.id == id) {
			return &buffer;
		}
	}

	return nullptr;
}

bool editor_destroy_buffer(Editor_State* editor, Buffer_ID id) {
	for (size_t i = 0; i < editor->loaded_buffers.count; i++) {
		if (editor->loaded_buffers[i].id == id) {
			array_remove_index(&editor->loaded_buffers, i);
			return true;
		}
	}

	return false;
}