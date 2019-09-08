#include "input.h"
#include "editor.h"
#include "buffer_view.h"
#include "config.h"

bool exit_requested = false;

bool is_mouse_over;
bool was_mouse_over;

ch::Vector2 last_mouse_position;
ch::Vector2 current_mouse_position;

static const usize MAX_MB = 3;
static bool mb_down[MAX_MB];
static bool mb_pressed[MAX_MB];
static bool mb_released[MAX_MB];

static const usize MAX_KEYS = 256;
static bool keys_down[MAX_KEYS];
static bool keys_pressed[MAX_KEYS];
static bool keys_released[MAX_KEYS];

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
		keys_down[key] = true;
		keys_pressed[key] = true;
		if (focused_view) focused_view->on_key_pressed(key);
	};

	the_window.on_key_released = [](const ch::Window& window, u8 key) {
		keys_down[key] = false;
		keys_released[key] = true;
	};

	the_window.on_char_entered = [](const ch::Window& window, u32 c) {
		if (focused_view) focused_view->on_char_entered(c);
	};

	the_window.on_resize = [](const ch::Window& window) {
		on_window_resize_config();
	};

	the_window.on_maximized = [](const ch::Window& window) {
		on_window_maximized_config();
	};
}

void process_input() {
	last_mouse_position = current_mouse_position;
	was_mouse_over = is_mouse_over;
	ch::mem_zero(mb_pressed, sizeof(mb_pressed));
	ch::mem_zero(mb_released, sizeof(mb_released));
	ch::mem_zero(keys_released, sizeof(keys_released));
	ch::mem_zero(keys_pressed, sizeof(keys_pressed));

	ch::poll_events();

	ch::Vector2 u32_mouse_pos;
	is_mouse_over = the_window.get_mouse_position(&u32_mouse_pos);
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

bool is_key_down(u8 key) {
	return keys_down[key];
}

bool was_key_pressed(u8 key) {
	return keys_pressed[key];
}

bool was_key_released(u8 key) {
	return keys_released[key];
}
