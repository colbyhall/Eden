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
#include "ui.h"

#include <assert.h>
#include <stdio.h>

const float scroll_speed = 10.f;
Editor_State g_editor;

static void editor_exit_requested(void* owner, Event* event) {
	Editor_State* editor = (Editor_State*)owner;
	editor->is_running = false;
}

static Buffer_View* get_hovered_view(Editor_State* editor) {
	const float window_width = (float)os_window_width();
	const float window_height = (float)os_window_height();
	const Vector2 mouse_position = os_get_mouse_position();

	float x = 0.f;
	float y = 0.f;

	const size_t views_count = editor->views_count;
	for (size_t i = 0; i < views_count; i++) {
		const float width = window_width / (float)views_count;

		const float x0 = x;
		const float y0 = y;
		const float x1 = x0 + width;
		const float y1 = y0 + window_height;

		if (point_in_rect(mouse_position, x0, y0, x1, y1)) {
			return &editor->views[i];
		}

		x += width;
	}

	return nullptr;
}

static void editor_mouse_wheel_scrolled(void* owner, Event* event) {
	Editor_State* editor = (Editor_State*)owner;
	Buffer_View* view = get_hovered_view(editor);

	if (!view) return;

    if (editor->input_state.ctrl_is_down) {

        float current_scroll_lines = view->current_scroll_y / editor->loaded_font.size;
        float target_scroll_lines = view->target_scroll_y / editor->loaded_font.size;

        if (event->delta >= 0) {
            editor->loaded_font.size += 2;
        } else {
            editor->loaded_font.size -= 2;
        }

        editor->loaded_font.size = floorf(editor->loaded_font.size);

        font_pack_atlas(editor->loaded_font);

        view->current_scroll_y = current_scroll_lines * editor->loaded_font.size;
        view->target_scroll_y = target_scroll_lines * editor->loaded_font.size;

        return;
    }

	Buffer* buffer = get_buffer(view);
	assert(buffer);

	view->target_scroll_y -= event->delta;

	if (view->target_scroll_y < 0.f) view->target_scroll_y = 0.f;

	const float font_height = editor->loaded_font.size;
	const float buffer_height = (buffer->eol_table.count * font_height);
	const float max_scroll = buffer_height - font_height;
	if (view->target_scroll_y > max_scroll) view->target_scroll_y = max_scroll;
}

static void on_left_mouse_down(void* owner, Event* event) {
    Editor_State* editor = (Editor_State*)owner;
    Buffer_View* view = get_hovered_view(editor);

    if (view) {
        const size_t picked_index = pick_index(view, event->mouse_position);
        view->cursor = picked_index;
        view->selection = picked_index;
	    refresh_cursor_info(view, true);
    }
}

#define DEFAULT_FONT_SIZE 16.0f

void editor_init(Editor_State* editor) {
	// @NOTE(Colby): init systems here
	gl_init();
	draw_init();

	bind_event_listener(&editor->input_state, make_event_listener(editor, editor_exit_requested, ET_Exit_Requested));
	bind_event_listener(&editor->input_state, make_event_listener(editor, editor_mouse_wheel_scrolled, ET_Mouse_Wheel_Scrolled));
    bind_event_listener(&editor->input_state, make_event_listener(editor, on_left_mouse_down, ET_Mouse_Down));

	editor->loaded_font = font_load_from_os("consola.ttf");
    editor->loaded_font.size = DEFAULT_FONT_SIZE;
    font_pack_atlas(editor->loaded_font);
	
	Buffer* buffer = editor_create_buffer(editor);
	buffer_load_from_file(buffer, "src\\editor.cpp");

	editor_set_current_view(editor, 0);
	editor->current_view->id = buffer->id;

	editor->is_running = true;
}

void editor_loop(Editor_State* editor) {
	double last_frame_time = os_get_time();
	while (editor->is_running) {
		double current_time = os_get_time();
		editor->dt = (float)(current_time - last_frame_time);
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
	input->mouse_went_down = false;
	input->mouse_went_up = false;
	input->last_mouse_position = input->current_mouse_position;
	os_poll_window_events();
	input->current_mouse_position = os_get_mouse_position();
}

void editor_tick(Editor_State* editor, float dt) {
	for (int i = 0; i < editor->views_count; i++) {
		Buffer_View* view = &editor->views[i];
		view->current_scroll_y = finterpto(view->current_scroll_y, view->target_scroll_y, dt, scroll_speed);
	}

    if (editor->input_state.left_mouse_button_down) {
        const size_t picked_index = pick_index(editor->current_view, editor->input_state.current_mouse_position);
        editor->current_view->cursor = picked_index;
		refresh_cursor_info(editor->current_view, true);
    }
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

	{
		const size_t views_count = editor->views_count;
		float x = 0.f;
		float y = 0.f;
		for (size_t i = 0; i < views_count; i++) {
			const float width = window_width / (float)views_count;
			
			const float x0 = x;
			const float y0 = y;
			const float x1 = x0 + width;
			const float y1 = y0 + window_height;

			x += width;
			draw_buffer_view(&editor->views[i], x0, y0, x1, y1, editor->loaded_font);
		}
	}
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

void editor_set_current_view(Editor_State* editor, size_t view_index) {
	assert(view_index < editor->views_count);

	if (editor->current_view)
	{
		buffer_view_lost_focus(editor->current_view);
	}

	editor->current_view = &editor->views[view_index];
	buffer_view_gained_focus(editor->current_view);
}
