#include "platform/Input.hpp"

#include <optional>

namespace pf {

namespace {

std::optional<MouseButton> toMouseButton(Uint8 sdlButton) {
    switch (sdlButton) {
        case SDL_BUTTON_LEFT: return MouseButton::Left;
        case SDL_BUTTON_MIDDLE: return MouseButton::Middle;
        case SDL_BUTTON_RIGHT: return MouseButton::Right;
        default: return std::nullopt;
    }
}

}

void Input::beginFrame() {
    for (auto& button : buttons) {
        button.pressed = false;
        button.released = false;
    }
    for (auto& key : keys) {
        key.pressed = false;
        key.released = false;
    }
    previousMouse = mousePosition;
    wheelDelta = 0;
    typed.clear();
}

void Input::handle(const SDL_Event& event) {
    switch (event.type) {
        case SDL_EVENT_QUIT:
            quit = true;
            break;
        case SDL_EVENT_MOUSE_MOTION:
            mousePosition = {event.motion.x, event.motion.y};
            break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            if (auto button = toMouseButton(event.button.button)) press(buttons[index(*button)]);
            break;
        case SDL_EVENT_MOUSE_BUTTON_UP:
            if (auto button = toMouseButton(event.button.button)) release(buttons[index(*button)]);
            break;
        case SDL_EVENT_MOUSE_WHEEL:
            wheelDelta += event.wheel.y;
            break;
        case SDL_EVENT_KEY_DOWN:
            if (!event.key.repeat || event.key.scancode == SDL_SCANCODE_BACKSPACE) {
                press(keys[event.key.scancode]);
            }
            break;
        case SDL_EVENT_KEY_UP:
            release(keys[event.key.scancode]);
            break;
        case SDL_EVENT_TEXT_INPUT:
            typed += event.text.text;
            break;
        default:
            break;
    }
}

void Input::press(ButtonState& state) {
    state.down = true;
    state.pressed = true;
}

void Input::release(ButtonState& state) {
    state.down = false;
    state.released = true;
}

}
