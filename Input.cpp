#include "Input.h"

void Input::Init() {
	for (int code = SDL_SCANCODE_UNKNOWN; code < SDL_NUM_SCANCODES; code++) {
		keyboard_states[static_cast<SDL_Scancode>(code)] = INPUT_STATE_UP;
	}
}

void Input::ProcessEvent(const SDL_Event& e) {
	if (e.type == SDL_KEYDOWN) {
 		keyboard_states[e.key.keysym.scancode] = INPUT_STATE_JUST_BECAME_DOWN;
		just_became_down_scancodes.push_back(e.key.keysym.scancode);
	}
	else if (e.type == SDL_KEYUP) {
		keyboard_states[e.key.keysym.scancode] = INPUT_STATE_JUST_BECAME_UP;
		just_became_up_scancodes.push_back(e.key.keysym.scancode);
	}
	else if (e.type == SDL_MOUSEMOTION) {
		mouse_position = glm::vec2(e.motion.x, e.motion.y);
	}
	else if (e.type == SDL_MOUSEBUTTONDOWN) {
		mouse_button_states[e.button.button] = INPUT_STATE_JUST_BECAME_DOWN;
		just_became_down_buttons.push_back(e.button.button);
	}
	else if (e.type == SDL_MOUSEBUTTONUP) {
		mouse_button_states[e.button.button] = INPUT_STATE_JUST_BECAME_UP;
		just_became_up_buttons.push_back(e.button.button);
	}
	else if (e.type == SDL_MOUSEWHEEL) {
		mouse_scroll_this_frame = e.wheel.preciseY;
	}
}

void Input::LateUpdate() {
	for (const SDL_Scancode& code : just_became_down_scancodes) {
  		keyboard_states[code] = INPUT_STATE_DOWN;
	}
	just_became_down_scancodes.clear();
	for (const SDL_Scancode& code : just_became_up_scancodes) {
		keyboard_states[code] = INPUT_STATE_UP;
	}
	just_became_up_scancodes.clear();
	for (int button : just_became_down_buttons) {
		mouse_button_states[button] = INPUT_STATE_DOWN;
	}
	just_became_down_buttons.clear();

	for (int button : just_became_up_buttons) {
		mouse_button_states[button] = INPUT_STATE_UP;
	}
	just_became_up_buttons.clear();
	// Reset mouse scroll wheel to zero after each frame
	mouse_scroll_this_frame = 0.0f;
}

bool Input::GetKey(const std::string& keyName) {
	auto it = __keycode_to_scancode.find(keyName);
	if (it != __keycode_to_scancode.end()) {
		auto keyState = keyboard_states.find(it->second);
		if (keyState != keyboard_states.end()) {
			INPUT_STATE state = keyState->second;
			return state == INPUT_STATE_DOWN || state == INPUT_STATE_JUST_BECAME_DOWN;
		}

	}
	return false;
}

bool Input::GetKeyDown(const std::string& keyName) {
	auto it = __keycode_to_scancode.find(keyName);
	if (it != __keycode_to_scancode.end()) {
		auto keyState = keyboard_states.find(it->second);
		if (keyState != keyboard_states.end()) {
			INPUT_STATE state = keyState->second;
			return state == INPUT_STATE_JUST_BECAME_DOWN;
		}
	}
	return false;
}

bool Input::GetKeyUp(const std::string& keyName) {
	auto it = __keycode_to_scancode.find(keyName);
	if (it != __keycode_to_scancode.end()) {
		auto keyState = keyboard_states.find(it->second);
		if (keyState != keyboard_states.end()) {
			INPUT_STATE state = keyState->second;
			return state == INPUT_STATE_JUST_BECAME_UP;
		}
	}
	return false;
}

glm::vec2 Input::GetMousePosition() {
	return mouse_position;
}

bool Input::GetMouseButton(int button) {
	auto it = mouse_button_states.find(button);
	if (it != mouse_button_states.end()) {
		return it->second == INPUT_STATE_DOWN || it->second == INPUT_STATE_JUST_BECAME_DOWN;
	}
	return false;
}
bool Input::GetMouseButtonDown(int button) {
	auto it = mouse_button_states.find(button);
	if (it != mouse_button_states.end()) {
		return it->second == INPUT_STATE_JUST_BECAME_DOWN;
	}
	return false;
}
bool Input::GetMouseButtonUp(int button) {
	auto it = mouse_button_states.find(button);
	if (it != mouse_button_states.end()) {
		return it->second == INPUT_STATE_JUST_BECAME_UP;
	}
	return false;
}
float Input::GetMouseScrollDelta() {
	return mouse_scroll_this_frame;
}
