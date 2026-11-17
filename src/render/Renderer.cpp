#include "render/Renderer.hpp"

#include <cmath>

namespace pf {

Renderer::Renderer(SDL_Window* window, SDL_Renderer* sdlRenderer, const std::filesystem::path& fontDirectory)
    : window(window), renderer(sdlRenderer), pixelFont(sdlRenderer, fontDirectory / "pixel8.font"), typography(sdlRenderer, fontDirectory) {}

void Renderer::beginFrame(Color clearColor) {
    pixelDensity = SDL_GetWindowPixelDensity(window);
    SDL_SetRenderTarget(renderer, nullptr);
    SDL_SetRenderScale(renderer, pixelDensity, pixelDensity);
    SDL_SetRenderClipRect(renderer, nullptr);
    clipStack.clear();
    setColor(clearColor);
    SDL_RenderClear(renderer);
    frameDrawCalls = 0;
}

void Renderer::endFrame() {
    SDL_RenderPresent(renderer);
    lastFrameDrawCalls = frameDrawCalls;
}

bool Renderer::capture(const std::filesystem::path& path) {
    SDL_Surface* pixels = SDL_RenderReadPixels(renderer, nullptr);
    if (!pixels) return false;
    const bool saved = SDL_SaveBMP(pixels, path.string().c_str());
    SDL_DestroySurface(pixels);
    return saved;
}

void Renderer::fill(const Rect& rect, Color color) {
    setColor(color);
    const SDL_FRect area{rect.x, rect.y, rect.w, rect.h};
    SDL_RenderFillRect(renderer, &area);
    ++frameDrawCalls;
}

void Renderer::outline(const Rect& rect, Color color) {
    setColor(color);
    const SDL_FRect area{rect.x, rect.y, rect.w, rect.h};
    SDL_RenderRect(renderer, &area);
    ++frameDrawCalls;
}

void Renderer::dashedOutline(const Rect& rect, Color color, float dash) {
    setColor(color);
    for (float x = rect.x; x < rect.right(); x += dash * 2) {
        const float end = std::fmin(x + dash, rect.right());
        SDL_RenderLine(renderer, x, rect.y, end, rect.y);
        SDL_RenderLine(renderer, x, rect.bottom() - 1, end, rect.bottom() - 1);
    }
    for (float y = rect.y; y < rect.bottom(); y += dash * 2) {
        const float end = std::fmin(y + dash, rect.bottom());
        SDL_RenderLine(renderer, rect.x, y, rect.x, end);
        SDL_RenderLine(renderer, rect.right() - 1, y, rect.right() - 1, end);
    }
    ++frameDrawCalls;
}

void Renderer::line(Vec2 from, Vec2 to, Color color) {
    setColor(color);
    SDL_RenderLine(renderer, from.x, from.y, to.x, to.y);
    ++frameDrawCalls;
}

void Renderer::polygon(std::span<const Vec2> points, Color color) {
    setColor(color);
    for (std::size_t index = 0; index < points.size(); ++index) {
        const Vec2 from = points[index];
        const Vec2 to = points[(index + 1) % points.size()];
        SDL_RenderLine(renderer, from.x, from.y, to.x, to.y);
    }
    ++frameDrawCalls;
}

void Renderer::text(Vec2 position, std::string_view content, Color color, TextStyle style) {
    typography.draw(position, content, color, style);
    ++frameDrawCalls;
}

void Renderer::pixelText(Vec2 position, std::string_view content, Color color, float scale) {
    SDL_Texture* atlas = pixelFont.atlas();
    if (!atlas) return;
    SDL_SetTextureColorMod(atlas, color.r, color.g, color.b);
    SDL_SetTextureAlphaMod(atlas, color.a);
    const float advance = BitmapFont::glyphSize * scale;
    SDL_FRect destination{std::round(position.x), std::round(position.y), advance, advance};
    const char* cursor = content.data();
    std::size_t remaining = content.size();
    while (remaining > 0) {
        const Uint32 codepoint = SDL_StepUTF8(&cursor, &remaining);
        if (codepoint > ' ') {
            const SDL_FRect source = pixelFont.source(codepoint);
            SDL_RenderTexture(renderer, atlas, &source, &destination);
        }
        destination.x += advance;
    }
    ++frameDrawCalls;
}

void Renderer::texture(SDL_Texture* texture, const Rect& destination, bool flipX) {
    const SDL_FRect area{destination.x, destination.y, destination.w, destination.h};
    SDL_RenderTextureRotated(renderer, texture, nullptr, &area, 0, nullptr, flipX ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE);
    ++frameDrawCalls;
}

void Renderer::pushClip(const Rect& rect) {
    clipStack.push_back(rect);
    applyClip();
}

void Renderer::popClip() {
    if (!clipStack.empty()) {
        clipStack.pop_back();
    }
    applyClip();
}

void Renderer::setTarget(SDL_Texture* target) {
    SDL_SetRenderTarget(renderer, target);
    const float scale = target ? 1 : pixelDensity;
    SDL_SetRenderScale(renderer, scale, scale);
    if (target) {
        SDL_SetRenderClipRect(renderer, nullptr);
    } else {
        applyClip();
    }
}

Vec2 Renderer::outputSize() const {
    int width = 0;
    int height = 0;
    SDL_GetWindowSize(window, &width, &height);
    return {static_cast<float>(width), static_cast<float>(height)};
}

void Renderer::setColor(Color color) {
    SDL_SetRenderDrawBlendMode(renderer, color.a == 255 ? SDL_BLENDMODE_NONE : SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
}

void Renderer::applyClip() {
    if (clipStack.empty()) {
        SDL_SetRenderClipRect(renderer, nullptr);
        return;
    }
    const Rect& clip = clipStack.back();
    const SDL_Rect area{static_cast<int>(clip.x), static_cast<int>(clip.y), static_cast<int>(clip.w),
                        static_cast<int>(clip.h)};
    SDL_SetRenderClipRect(renderer, &area);
}

}
