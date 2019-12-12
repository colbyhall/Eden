#pragma once

#include <ch_stl/input.h>
#include <ch_stl/math.h>
#include <ch_stl/array.h>
#include <ch_stl/hash.h>

extern ch::Vector2 last_mouse_position;
extern ch::Vector2 current_mouse_position;
extern f32 current_mouse_scroll_y;

enum Key_Bind_Modifier {
	KBM_None  = 0,
	KBM_Shift = 1,
	KBM_Ctrl  = 1 << 1,
	KBM_Alt   = 1 << 2
};

struct Key_Bind {
	u8 modifier_flags;
	u32 key;

	Key_Bind() : modifier_flags(0), key(0) {}
	Key_Bind(u8 _modifier_flags, u32 _key) : modifier_flags(_modifier_flags), key(_key) {}

	CH_FORCEINLINE bool operator==(const Key_Bind bind) const {
		return modifier_flags == bind.modifier_flags && key == bind.key;
	}
};

CH_FORCEINLINE u64 hash(const Key_Bind bind) {
	return ch::fnv1_hash(&bind, sizeof(Key_Bind));
}

using Action_Func = void (*)();

/**
 * Binds a function to a Key_Bind
 *
 * @see unbind_action
 * 
 * @param binding used as a key to find the action
 * @param action called when binding has been called
 * @returns true if the binding was overridden
 */
bool bind_action(const Key_Bind binding, Action_Func action);

/**
 * Removes an action binding from all bindings
 *
 * @see bind_action
 *
 * @param binding is used to find the action and remove it
 * @returns true if the binding was found and removed
 */
bool unbind_action(const Key_Bind binding);

/**
 * Checks if a binding is currently bound
 *
 * @returns true if the binding is currently bound
 */
bool has_action_binding(const Key_Bind binding);

/**
 * Removes all bindings
 * 
 * @returns true if a binding was removed. 
 */
bool remove_all_bindings();

/** Inits the input system. */
void init_input();

/** Binds all the default bindings. */
void setup_default_bindings();

/** Polls input from the window and executes actions in the action table. */
void process_input();

/** @returns true if exit button on window was pressed. */
bool is_exit_requested();

/** @returns true if a mouse button is down. */
bool is_mouse_button_down(u8 mb);

/** @returns true if a mouse button was pressed this frame. */
bool was_mouse_button_pressed(u8 mb);

/** @returns true if a mouse button was released this frame. */
bool was_mouse_button_released(u8 mb);
