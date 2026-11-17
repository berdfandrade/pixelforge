#pragma once

#include "game/Projection.hpp"
#include "render/Renderer.hpp"
#include "render/SpriteLibrary.hpp"
#include "scene/Scene.hpp"

#include <string_view>
#include <vector>

namespace pf {

class GameRenderer {
public:
    GameRenderer(Renderer& renderer, const SpriteLibrary& sprites);

    void render(const Scene& scene, Vec2 camera, Vec2 viewSize, bool showHud);
    SDL_Texture* output() const { return target.get(); }
    Vec2 outputSize() const { return size; }

    Vec2 spriteSize(const Entity& entity) const;
    Rect spriteBounds(const Entity& entity, const Projection& projection) const;
    std::vector<const Entity*> drawOrder(const Scene& scene) const;

private:
    void ensureTarget(Vec2 viewSize);
    void drawSideBackdrop(const Backdrop& backdrop, Vec2 camera);
    void drawSideTiles(const Tilemap& tilemap, Vec2 camera);
    void drawTopDownTiles(const Tilemap& tilemap, Vec2 camera);
    void drawIsometric(const Scene& scene, Vec2 camera);
    void drawEntity(const Entity& entity, const Projection& projection, Vec2 camera);
    void drawSprite(const char* asset, std::string_view animation, Vec2 position);
    void drawHud(const GameState& state);

    Renderer& renderer;
    const SpriteLibrary& sprites;
    TextureHandle target;
    Vec2 size;
};

}
