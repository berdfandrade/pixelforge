#pragma once

#include <SDL3/SDL.h>

#include <memory>

namespace pf {

struct TextureDeleter {
    void operator()(SDL_Texture* texture) const { SDL_DestroyTexture(texture); }
};

using TextureHandle = std::unique_ptr<SDL_Texture, TextureDeleter>;

}
