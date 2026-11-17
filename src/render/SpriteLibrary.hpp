#pragma once

#include "render/Texture.hpp"

#include <SDL3/SDL.h>

#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace pf {

struct Animation {
    float framesPerSecond = 1;
    std::vector<TextureHandle> frames;
};

struct SpriteAsset {
    std::string name;
    int width = 0;
    int height = 0;
    std::map<std::string, Animation> animations;

    SDL_Texture* frame(const std::string& animation, float time) const;
    SDL_Texture* preview() const;
};

class SpriteLibrary {
public:
    SpriteLibrary(SDL_Renderer* renderer, std::filesystem::path directory);

    void reload();
    const SpriteAsset* find(const std::string& name) const;
    const std::vector<SpriteAsset>& all() const { return sprites; }

private:
    SDL_Renderer* renderer;
    std::filesystem::path directory;
    std::vector<SpriteAsset> sprites;
};

}
