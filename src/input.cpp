#include "input.h"
#include "keys.h"

Event make_window_resize_event(u32 old_width, u32 old_height) {
	Event result = {};
	result.type = ET_Window_Resize;
	result.old_width = old_width;
	result.old_height = old_height;

	return result;
}

Event make_exit_requested_event() {
	Event result = {};
	result.type = ET_Exit_Requested;
	return result;
}

Event make_mouse_down_event(Vector2 mouse_position) {
	Event result = {};
	result.type = ET_Mouse_Down;
	result.mouse_position = mouse_position;
	return result;
}

Event make_mouse_up_event(Vector2 mouse_position) {
	Event result = {};
	result.type = ET_Mouse_Up;
	result.mouse_position = mouse_position;
	return result;
}

Event make_mouse_moved_event(Vector2 mouse_position) {
	Event result = {};
	result.type = ET_Mouse_Moved;
	result.mouse_position = mouse_position;
	return result;
}

Event make_mouse_wheel_scrolled_event(float delta) {
	Event result = {};
	result.type = ET_Mouse_Wheel_Scrolled;
	result.delta = delta;
	return result;
}

Event make_key_pressed_event(u8 key_code) {
	Event result = {};
	result.type = ET_Key_Pressed;
	result.key_code = key_code;
	return result;
}


Event make_key_released_event(u8 key_code) {
	Event result = {};
	result.type = ET_Key_Released;
	result.key_code = key_code;
	return result;
}

Event make_char_entered_event(u32 c) {
	Event result = {};
	result.type = ET_Char_Entered;
	result.c = c;
	return result;
}

Event_Listener make_event_listener(void* owner, Process_Event process_event, Event_Type type) {
	Event_Listener result = {};
	result.owner = owner;
	result.process_func = process_event;
	result.type = type;
	assert(result);
	return result;
}

void process_input_event(Input_State* input, Event* event) {
	for (Event_Listener& el : input->event_listeners) {
		if (el.type == event->type || el.type == ET_None) {
			el.process_func(el.owner, event);
		}

		switch (event->type) {
		case ET_Mouse_Down:
			input->mouse_went_down = true;
            input->left_mouse_button_down = true;
			break;
		case ET_Mouse_Up:
			input->mouse_went_up = true;
            input->left_mouse_button_down = false;
			break;
		case ET_Key_Pressed:
		case ET_Key_Released:
			const u32 key_code = event->key_code;
			if (key_code == KEY_CTRL) {
				input->ctrl_is_down = event->type == ET_Key_Pressed;
			} else if (key_code == KEY_ALT) {
				input->alt_is_down = event->type == ET_Key_Pressed;
			} else if (key_code == KEY_SHIFT) {
                input->shift_is_down = event->type == ET_Key_Pressed;
            }
			break;
		}
	}
}

bool bind_event_listener(Input_State* input, const Event_Listener& event_listener) {
	if (input->event_listeners.contains(event_listener)) {
		return false;
	}

	assert(event_listener);
	input->event_listeners.push(event_listener);
	return true;
}

bool unbind_event_listener(Input_State* input, const Event_Listener& event_listener) {
    for (usize i = 0; i < input->event_listeners.count; i++) {
        const Event_Listener& el = input->event_listeners[i];
        if (el == event_listener) {
            input->event_listeners.remove(i);
            return true;
        }
    }

    return false;
}

Input_State::Input_State() {
    event_listeners.reserve(1024);
}
