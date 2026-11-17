#pragma once

#include "platform/Input.hpp"

namespace pf {

struct GameInput {
    bool left = false;
    bool right = false;
    bool up = false;
    bool down = false;
    bool jumpPressed = false;
    bool jumpHeld = false;

    static bool jumpKeyPressed(const Input& input) {
        return input.keyPressed(SDL_SCANCODE_SPACE) || input.keyPressed(SDL_SCANCODE_UP) ||
               input.keyPressed(SDL_SCANCODE_W) || input.keyPressed(SDL_SCANCODE_Z);
    }

    static GameInput read(const Input& input, bool jumpPressed) {
        return {input.keyDown(SDL_SCANCODE_LEFT) || input.keyDown(SDL_SCANCODE_A),
                input.keyDown(SDL_SCANCODE_RIGHT) || input.keyDown(SDL_SCANCODE_D),
                input.keyDown(SDL_SCANCODE_UP) || input.keyDown(SDL_SCANCODE_W),
                input.keyDown(SDL_SCANCODE_DOWN) || input.keyDown(SDL_SCANCODE_S), jumpPressed,
                input.keyDown(SDL_SCANCODE_SPACE) || input.keyDown(SDL_SCANCODE_UP) || input.keyDown(SDL_SCANCODE_W) ||
                    input.keyDown(SDL_SCANCODE_Z)};
    }
};

}
