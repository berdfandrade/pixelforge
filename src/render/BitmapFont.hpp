#pragma once

#include "render/Texture.hpp"

#include <SDL3/SDL.h>

#include <array>
#include <filesystem>

namespace pf {

class BitmapFont {
public:
    static constexpr int glyphSize = 8;
    static constexpr int glyphCount = 256;

    BitmapFont(SDL_Renderer* renderer, const std::filesystem::path& path);

    SDL_Texture* atlas() const { return texture.get(); }
    SDL_FRect source(Uint32 codepoint) const;

private:
    std::array<bool, glyphCount> available{};
    TextureHandle texture;
};

}
