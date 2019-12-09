#include "input.h"
#include "editor.h"
#include "buffer_view.h"
#include "actions.h"

#include <ch_stl/hash_table.h>

static bool exit_requested = false;

ch::Vector2 last_mouse_position;
ch::Vector2 current_mouse_position;
f32 current_mouse_scroll_y;

static const usize MAX_MB = 3;
static bool mb_down[MAX_MB];
static bool mb_pressed[MAX_MB];
static bool mb_released[MAX_MB];

static u8 current_key_modifiers = KBM_None;

static ch::Hash_Table<Key_Bind, Action_Func> action_table;

bool bind_action(const Key_Bind binding, Action_Func action) {
	assert(action);

	Action_Func* const found_action = action_table.find(binding);
	if (found_action) {
		*found_action = action;
		return true;
	}

	action_table.push(binding, action);
	return false;
}

bool unbind_action(const Key_Bind binding) {
	return action_table.remove(binding);
}

bool has_action_binding(const Key_Bind binding) {
	return action_table.contains(binding);
}

bool remove_all_bindings() {
	if (action_table.buckets.count) {
		action_table.free();
		return true;
	}

	return false;
}

void init_input() {
	the_window.on_exit_requested = [](const ch::Window& window) {
		exit_requested = true;
	};

	the_window.on_mouse_button_down = [](const ch::Window& window, u8 mouse_button) {
		mb_down[mouse_button] = true;
		mb_pressed[mouse_button] = true;
	};

	the_window.on_mouse_button_up = [](const ch::Window& window, u8 mouse_button) {
		mb_down[mouse_button] = false;
		mb_released[mouse_button] = true;
	};

	the_window.on_key_pressed = [](const ch::Window& window, u8 key) {
		switch (key) {
		case CH_KEY_SHIFT:
			current_key_modifiers |= KBM_Shift;
			break;
		case CH_KEY_CONTROL:
			current_key_modifiers |= KBM_Ctrl;
			break;
		case CH_KEY_ALT:
			current_key_modifiers |= KBM_Alt;
			break;
		default:
			const Key_Bind current_binding(current_key_modifiers, key);

			Action_Func* const action = action_table.find(current_binding);
			if (action && *action) {
				(*action)();
			}
			break;
		}
	};

	the_window.on_key_released = [](const ch::Window& window, u8 key) {
		switch (key) {
		case CH_KEY_SHIFT:
			current_key_modifiers &= ~KBM_Shift;		
			break;
		case CH_KEY_CONTROL:
			current_key_modifiers &= ~KBM_Ctrl;
			break;
		case CH_KEY_ALT:
			current_key_modifiers &= ~KBM_Alt;
			break;
		}
	};

	the_window.on_char_entered = [](const ch::Window& window, u32 c) {
		Buffer_View* const focused_view = get_focused_view();
		if (focused_view) {
			focused_view->on_char_entered(c);
		}
	};

	the_window.on_mouse_wheel_scrolled = [](const ch::Window& window, f32 delta) {
		current_mouse_scroll_y = delta;
	};

	setup_default_bindings();
}

void setup_default_bindings() {
	bind_action(Key_Bind(KBM_None, CH_KEY_BACKSPACE), backspace);
	bind_action(Key_Bind(KBM_Shift, CH_KEY_BACKSPACE), backspace);

	bind_action(Key_Bind(KBM_None, CH_KEY_ENTER), newline);
	bind_action(Key_Bind(KBM_Shift, CH_KEY_ENTER), newline);

	bind_action(Key_Bind(KBM_None, CH_KEY_LEFT), move_cursor_left);
	bind_action(Key_Bind(KBM_None, CH_KEY_RIGHT), move_cursor_right);
	bind_action(Key_Bind(KBM_None, CH_KEY_UP), move_cursor_up);
	bind_action(Key_Bind(KBM_None, CH_KEY_DOWN), move_cursor_down);

	bind_action(Key_Bind(KBM_Ctrl, CH_KEY_S), save_buffer);
}

void process_input() {
	last_mouse_position = current_mouse_position;
	ch::mem_zero(mb_pressed, sizeof(mb_pressed));
	ch::mem_zero(mb_released, sizeof(mb_released));
	current_mouse_scroll_y = 0.f;

	ch::wait_events();

	ch::Vector2 u32_mouse_pos;
	the_window.get_mouse_position(&u32_mouse_pos);
	current_mouse_position.x = (f32)u32_mouse_pos.ux;
	current_mouse_position.y = (f32)u32_mouse_pos.uy;
}

bool is_exit_requested() {
	return exit_requested;
}

bool is_mouse_button_down(u8 mb) {
	return mb_down[mb];
}

bool was_mouse_button_pressed(u8 mb) {
	return mb_pressed[mb];
}

bool was_mouse_button_released(u8 mb) {
	return mb_released[mb];
}