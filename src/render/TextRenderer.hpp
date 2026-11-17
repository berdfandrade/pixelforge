#pragma once

#include "core/Color.hpp"
#include "core/Geometry.hpp"

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <array>
#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>

namespace pf {

enum class TextStyle { Body, Strong, Mono };

class TextRenderer {
public:
    TextRenderer(SDL_Renderer* renderer, const std::filesystem::path& fontDirectory);
    ~TextRenderer();

    TextRenderer(const TextRenderer&) = delete;
    TextRenderer& operator=(const TextRenderer&) = delete;

    void setSizes(float bodySize, float monoSize);
    void draw(Vec2 position, std::string_view content, Color color, TextStyle style);
    float width(std::string_view content, TextStyle style) const;
    float lineHeight(TextStyle style) const;

private:
    TTF_Font* font(TextStyle style) const { return fonts[static_cast<std::size_t>(style)]; }
    TTF_Text* shaped(std::string_view content, TextStyle style);
    void clearCache();

    TTF_TextEngine* engine = nullptr;
    std::array<TTF_Font*, 3> fonts{};
    std::unordered_map<std::string, TTF_Text*> cache;
};

}
