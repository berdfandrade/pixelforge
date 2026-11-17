#include "render/TextRenderer.hpp"

#include <cmath>
#include <stdexcept>

namespace pf {

namespace {

constexpr std::size_t maxCachedTexts = 4096;
constexpr float defaultBodySize = 13;
constexpr float defaultMonoSize = 12;

constexpr std::array<const char*, 3> fontFiles{"Inter-Regular.ttf", "Inter-SemiBold.ttf", "JetBrainsMono-Regular.ttf"};

TTF_Font* openFont(const std::filesystem::path& path, float size) {
    TTF_Font* font = TTF_OpenFont(path.string().c_str(), size);
    if (!font) throw std::runtime_error("Fonte não carregada: " + path.string() + " (" + SDL_GetError() + ")");
    TTF_SetFontHinting(font, TTF_HINTING_LIGHT_SUBPIXEL);
    return font;
}

}

TextRenderer::TextRenderer(SDL_Renderer* renderer, const std::filesystem::path& fontDirectory) {
    if (!TTF_Init()) throw std::runtime_error(std::string("TTF_Init: ") + SDL_GetError());
    engine = TTF_CreateRendererTextEngine(renderer);
    if (!engine) throw std::runtime_error(std::string("TTF_CreateRendererTextEngine: ") + SDL_GetError());
    for (std::size_t index = 0; index < fonts.size(); ++index) {
        const float size = static_cast<TextStyle>(index) == TextStyle::Mono ? defaultMonoSize : defaultBodySize;
        fonts[index] = openFont(fontDirectory / fontFiles[index], size);
    }
}

TextRenderer::~TextRenderer() {
    clearCache();
    for (TTF_Font* font : fonts) {
        if (font) TTF_CloseFont(font);
    }
    if (engine) TTF_DestroyRendererTextEngine(engine);
    TTF_Quit();
}

void TextRenderer::setSizes(float bodySize, float monoSize) {
    clearCache();
    TTF_SetFontSize(font(TextStyle::Body), bodySize);
    TTF_SetFontSize(font(TextStyle::Strong), bodySize);
    TTF_SetFontSize(font(TextStyle::Mono), monoSize);
}

void TextRenderer::draw(Vec2 position, std::string_view content, Color color, TextStyle style) {
    if (content.empty()) return;
    TTF_Text* text = shaped(content, style);
    if (!text) return;
    TTF_SetTextColor(text, color.r, color.g, color.b, color.a);
    TTF_DrawRendererText(text, std::round(position.x), std::round(position.y));
}

float TextRenderer::width(std::string_view content, TextStyle style) const {
    if (content.empty()) return 0;
    int measured = 0;
    int height = 0;
    TTF_GetStringSize(font(style), content.data(), content.size(), &measured, &height);
    return static_cast<float>(measured);
}

float TextRenderer::lineHeight(TextStyle style) const { return static_cast<float>(TTF_GetFontHeight(font(style))); }

TTF_Text* TextRenderer::shaped(std::string_view content, TextStyle style) {
    std::string key(1, static_cast<char>(style));
    key.append(content);
    if (const auto found = cache.find(key); found != cache.end()) return found->second;
    if (cache.size() >= maxCachedTexts) clearCache();
    TTF_Text* text = TTF_CreateText(engine, font(style), content.data(), content.size());
    if (text) cache.emplace(std::move(key), text);
    return text;
}

void TextRenderer::clearCache() {
    for (auto& [key, text] : cache) TTF_DestroyText(text);
    cache.clear();
}

}
