#pragma once

#include "core/Color.hpp"
#include "core/Geometry.hpp"
#include "render/BitmapFont.hpp"
#include "render/TextRenderer.hpp"

#include <SDL3/SDL.h>

#include <filesystem>
#include <span>
#include <string_view>
#include <vector>

namespace pf {

class Renderer {
public:
    Renderer(SDL_Window* window, SDL_Renderer* sdlRenderer, const std::filesystem::path& fontDirectory);

    void beginFrame(Color clearColor);
    void endFrame();
    bool capture(const std::filesystem::path& path);

    void fill(const Rect& rect, Color color);
    void outline(const Rect& rect, Color color);
    void dashedOutline(const Rect& rect, Color color, float dash);
    void line(Vec2 from, Vec2 to, Color color);
    void polygon(std::span<const Vec2> points, Color color);
    void text(Vec2 position, std::string_view content, Color color, TextStyle style = TextStyle::Body);
    void pixelText(Vec2 position, std::string_view content, Color color, float scale = 1);
    void texture(SDL_Texture* texture, const Rect& destination, bool flipX = false);

    void pushClip(const Rect& rect);
    void popClip();

    void setTarget(SDL_Texture* target);

    float textWidth(std::string_view content, TextStyle style = TextStyle::Body) const { return typography.width(content, style); }
    float lineHeight(TextStyle style = TextStyle::Body) const { return typography.lineHeight(style); }
    void setFontSizes(float bodySize, float monoSize) { typography.setSizes(bodySize, monoSize); }

    SDL_Renderer* native() const { return renderer; }
    int drawCalls() const { return lastFrameDrawCalls; }
    Vec2 outputSize() const;

private:
    void setColor(Color color);
    void applyClip();

    SDL_Window* window;
    SDL_Renderer* renderer;
    BitmapFont pixelFont;
    TextRenderer typography;
    float pixelDensity = 1;
    std::vector<Rect> clipStack;
    int frameDrawCalls = 0;
    int lastFrameDrawCalls = 0;
};

}
