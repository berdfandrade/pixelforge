#include "render/BitmapFont.hpp"

#include "core/Log.hpp"

#include <cstdint>
#include <fstream>
#include <sstream>

namespace pf {

namespace {

constexpr int atlasColumns = 16;
constexpr Uint32 fallbackGlyph = '?';

void plotGlyph(SDL_Surface* surface, int codepoint, const std::array<std::uint8_t, 8>& rows) {
    const int originX = (codepoint % atlasColumns) * BitmapFont::glyphSize;
    const int originY = (codepoint / atlasColumns) * BitmapFont::glyphSize;
    for (int y = 0; y < BitmapFont::glyphSize; ++y) {
        auto* line = reinterpret_cast<std::uint32_t*>(static_cast<std::uint8_t*>(surface->pixels) + (originY + y) * surface->pitch);
        for (int x = 0; x < BitmapFont::glyphSize; ++x) {
            line[originX + x] = (rows[y] >> x & 1) ? 0xffffffffu : 0u;
        }
    }
}

}

BitmapFont::BitmapFont(SDL_Renderer* renderer, const std::filesystem::path& path) {
    const int atlasSize = atlasColumns * glyphSize;
    SDL_Surface* surface = SDL_CreateSurface(atlasSize, atlasSize, SDL_PIXELFORMAT_RGBA8888);
    if (!surface) return;
    SDL_FillSurfaceRect(surface, nullptr, 0);

    std::ifstream file(path);
    if (!file) Log::error("Fonte não encontrada: " + path.string());
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream words(line);
        int codepoint = 0;
        words >> std::hex >> codepoint;
        std::array<std::uint8_t, 8> rows{};
        for (auto& row : rows) {
            int value = 0;
            words >> value;
            row = static_cast<std::uint8_t>(value);
        }
        if (!words || codepoint < 0 || codepoint >= glyphCount) continue;
        plotGlyph(surface, codepoint, rows);
        available[static_cast<std::size_t>(codepoint)] = true;
    }

    texture.reset(SDL_CreateTextureFromSurface(renderer, surface));
    SDL_DestroySurface(surface);
    if (texture) SDL_SetTextureScaleMode(texture.get(), SDL_SCALEMODE_NEAREST);
}

SDL_FRect BitmapFont::source(Uint32 codepoint) const {
    const Uint32 glyph = codepoint < glyphCount && available[codepoint] ? codepoint : fallbackGlyph;
    return {static_cast<float>(glyph % atlasColumns * glyphSize), static_cast<float>(glyph / atlasColumns * glyphSize),
            static_cast<float>(glyphSize), static_cast<float>(glyphSize)};
}

}
