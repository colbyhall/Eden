#pragma once

#include "types.h"
#include "math.h"
#include "array.h"

enum Event_Type {
	ET_None,
	ET_Window_Resize,
	ET_Exit_Requested,
	ET_Mouse_Down,
	ET_Mouse_Up,
	ET_Mouse_Moved,
	ET_Mouse_Wheel_Scrolled,
	ET_Key_Pressed,
	ET_Key_Released,
	ET_Char_Entered,
};

struct Event {
	Event_Type type;

	union {
		struct {
			u32 old_width;
			u32 old_height;
		};
		Vector2 mouse_position;
		float delta;
		u8 key_code;
		u8 c;
	};

	bool handled;
};

Event make_window_resize_event(u32 old_width, u32 old_height);
Event make_exit_requested_event();
Event make_mouse_down_event(Vector2 mouse_position);
Event make_mouse_up_event(Vector2 mouse_position);
Event make_mouse_moved_event(Vector2 mouse_position);
Event make_mouse_wheel_scrolled_event(float delta);
Event make_key_pressed_event(u8 key_code);
Event make_key_released_event(u8 key_code);
Event make_char_entered_event(u8 c);

using Process_Event = void(*)(void* owner, Event* event);

struct Event_Listener {
	void* owner;
	Process_Event process_func;
	Event_Type type;

	explicit operator bool() const { return owner && process_func; }
	bool operator==(const Event_Listener& right) const { return owner == right.owner && process_func == right.process_func; }
};

Event_Listener make_event_listener(void* owner, Process_Event process_event, Event_Type type);

struct Input_State {
	bool mouse_went_down = false;
	bool mouse_went_up = false;

	Vector2 last_mouse_position;
	Vector2 current_mouse_position;

	u8 last_key_pressed = 0;

	bool ctrl_is_down = false;
	bool alt_is_down = false;

	Array<Event_Listener> event_listeners;
};

void process_input_event(Input_State* input, Event* event);

bool bind_event_listener(Input_State* input, const Event_Listener* event_listener);
bool unbind_event_listener(Input_State* input, const Event_Listener* event_listener);