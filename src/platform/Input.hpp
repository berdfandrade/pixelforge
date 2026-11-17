#pragma once

#include "core/Geometry.hpp"

#include <SDL3/SDL.h>

#include <array>
#include <string>

namespace pf {

enum class MouseButton { Left, Middle, Right };

class Input {
public:
    void beginFrame();
    void handle(const SDL_Event& event);

    Vec2 mouse() const { return mousePosition; }
    Vec2 mouseDelta() const { return mousePosition - previousMouse; }
    float wheel() const { return wheelDelta; }
    bool mouseDown(MouseButton button) const { return buttons[index(button)].down; }
    bool mousePressed(MouseButton button) const { return buttons[index(button)].pressed; }
    bool mouseReleased(MouseButton button) const { return buttons[index(button)].released; }

    bool keyDown(SDL_Scancode key) const { return keys[key].down; }
    bool keyPressed(SDL_Scancode key) const { return keys[key].pressed; }
    bool ctrl() const { return keyDown(SDL_SCANCODE_LCTRL) || keyDown(SDL_SCANCODE_RCTRL); }
    bool shift() const { return keyDown(SDL_SCANCODE_LSHIFT) || keyDown(SDL_SCANCODE_RSHIFT); }

    const std::string& typedText() const { return typed; }
    bool quitRequested() const { return quit; }
    void cancelQuit() { quit = false; }

private:
    struct ButtonState {
        bool down = false;
        bool pressed = false;
        bool released = false;
    };

    static std::size_t index(MouseButton button) { return static_cast<std::size_t>(button); }
    static void press(ButtonState& state);
    static void release(ButtonState& state);

    std::array<ButtonState, 3> buttons{};
    std::array<ButtonState, SDL_SCANCODE_COUNT> keys{};
    Vec2 mousePosition;
    Vec2 previousMouse;
    float wheelDelta = 0;
    std::string typed;
    bool quit = false;
};

}
