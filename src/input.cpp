#include "input.h"
#include "editor.h"
#include "buffer_view.h"

bool exit_requested = false;

bool is_mouse_over;
bool was_mouse_over;

ch::Vector2 last_mouse_position;
ch::Vector2 current_mouse_position;

static const usize MAX_MB = 3;
static bool mb_down[MAX_MB];
static bool mb_went_down[MAX_MB];
static bool mb_went_up[MAX_MB];

static const usize MAX_KEYS = 256;
static bool keys_down[MAX_KEYS];
static bool keys_went_down[MAX_KEYS];
static bool keys_went_up[MAX_KEYS];

void init_input() {
	the_window.on_exit_requested = [](const ch::Window& window) {
		exit_requested = true;
	};

	the_window.on_mouse_button_down = [](const ch::Window& window, u8 mouse_button) {
		mb_down[mouse_button] = true;
		mb_went_down[mouse_button] = true;
	};

	the_window.on_mouse_button_up = [](const ch::Window& window, u8 mouse_button) {
		mb_down[mouse_button] = false;
		mb_went_up[mouse_button] = true;
	};

	the_window.on_key_pressed = [](const ch::Window& window, u8 key) {
		keys_down[key] = true;
		keys_went_down[key] = true;
	};

	the_window.on_key_released = [](const ch::Window& window, u8 key) {
		keys_down[key] = false;
		keys_went_up[key] = true;
	};

	the_window.on_char_entered = [](const ch::Window& window, u32 c) {
		if (focused_view) focused_view->on_char_entered(c);
	};
}

void process_input() {
	last_mouse_position = current_mouse_position;
	was_mouse_over = is_mouse_over;
	ch::mem_zero(mb_went_down, sizeof(mb_went_down));
	ch::mem_zero(mb_went_up, sizeof(mb_went_up));
	ch::mem_zero(keys_went_up, sizeof(keys_went_up));
	ch::mem_zero(keys_went_down, sizeof(keys_went_down));

	ch::poll_events();

	ch::Vector2 u32_mouse_pos;
	is_mouse_over = the_window.get_mouse_position(&u32_mouse_pos);
	current_mouse_position.x = (f32)u32_mouse_pos.ux;
	current_mouse_position.y = (f32)u32_mouse_pos.uy;
}

bool is_exit_requested() {
	return exit_requested;
}

bool is_mb_down(u8 mb) {
	return mb_down[mb];
}

bool did_mb_go_down(u8 mb) {
	return mb_went_down[mb];
}

bool did_mb_go_up(u8 mb) {
	return mb_went_up[mb];
}

bool is_key_down(u8 key) {
	return keys_down[key];
}

bool did_key_go_down(u8 key) {
	return keys_went_down[key];
}

bool did_key_go_up(u8 key) {
	return keys_went_up[key];
}
