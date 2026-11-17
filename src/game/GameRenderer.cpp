#include "game/GameRenderer.hpp"

#include <algorithm>
#include <cmath>
#include <format>

namespace pf {

namespace {

constexpr float hillParallax = 0.4f;
constexpr int hillBlockWidth = 37;
constexpr Vec2 moonOffset{58, 22};
constexpr float moonSize = 14;
constexpr float isoTileWidth = Tilemap::tileSize * 2;
constexpr float isoBlockRise = 8;
constexpr float isoFootInset = 2;

float hillHeight(int block) {
    const unsigned hash = static_cast<unsigned>(block) * 2654435761u;
    return 40.0f + static_cast<float>((hash >> 16) % 6) * 9.0f;
}

Vec2 footprintCenter(const Entity& entity) {
    const Rect collider = entity.worldCollider();
    return {collider.x + collider.w / 2, collider.y + collider.h / 2};
}

float isoDepth(Vec2 world) { return world.x + world.y; }

int layerOf(const Entity& entity) { return entity.sprite ? entity.sprite->layer : 0; }

const char* alternating(int column, int row) { return (column + row) % 2 == 1 ? "alt" : "idle"; }

struct CellRange {
    int firstColumn;
    int lastColumn;
    int firstRow;
    int lastRow;
};

CellRange visibleCells(const Tilemap& tilemap, Vec2 camera, Vec2 size) {
    const int tile = Tilemap::tileSize;
    return {std::max(0, static_cast<int>(std::floor(camera.x / tile))),
            std::min(tilemap.columns() - 1, static_cast<int>(std::floor((camera.x + size.x) / tile))),
            std::max(0, static_cast<int>(std::floor(camera.y / tile))),
            std::min(tilemap.rows() - 1, static_cast<int>(std::floor((camera.y + size.y) / tile)))};
}

}

GameRenderer::GameRenderer(Renderer& renderer, const SpriteLibrary& sprites) : renderer(renderer), sprites(sprites) {}

void GameRenderer::render(const Scene& scene, Vec2 camera, Vec2 viewSize, bool showHud) {
    ensureTarget(viewSize);
    const Vec2 snapped{std::floor(camera.x), std::floor(camera.y)};
    const Projection projection = Projection::of(scene);
    renderer.setTarget(target.get());
    switch (scene.camera) {
        case CameraMode::Side:
            drawSideBackdrop(scene.backdrop, snapped);
            drawSideTiles(scene.tilemap, snapped);
            for (const Entity* entity : drawOrder(scene)) drawEntity(*entity, projection, snapped);
            break;
        case CameraMode::TopDown:
            renderer.fill({0, 0, size.x, size.y}, scene.backdrop.sky);
            drawTopDownTiles(scene.tilemap, snapped);
            for (const Entity* entity : drawOrder(scene)) drawEntity(*entity, projection, snapped);
            break;
        case CameraMode::Isometric:
            renderer.fill({0, 0, size.x, size.y}, scene.backdrop.sky);
            drawIsometric(scene, snapped);
            break;
    }
    if (showHud) drawHud(scene.state);
    renderer.setTarget(nullptr);
}

Vec2 GameRenderer::spriteSize(const Entity& entity) const {
    const SpriteAsset* asset = entity.sprite ? sprites.find(entity.sprite->asset) : nullptr;
    if (!asset) return {entity.worldCollider().w, entity.worldCollider().h};
    return {static_cast<float>(asset->width), static_cast<float>(asset->height)};
}

Rect GameRenderer::spriteBounds(const Entity& entity, const Projection& projection) const {
    const bool hasSprite = entity.sprite && sprites.find(entity.sprite->asset);
    if (!projection.isometric()) {
        if (!hasSprite) return entity.worldCollider();
        const Vec2 dimensions = spriteSize(entity);
        return {entity.transform.position.x, entity.transform.position.y, dimensions.x, dimensions.y};
    }
    if (!hasSprite) return projection.viewBounds(entity.worldCollider());
    const Vec2 dimensions = spriteSize(entity);
    const Vec2 foot = projection.toView(footprintCenter(entity));
    return {foot.x - dimensions.x / 2, foot.y - dimensions.y + isoFootInset, dimensions.x, dimensions.y};
}

std::vector<const Entity*> GameRenderer::drawOrder(const Scene& scene) const {
    std::vector<const Entity*> order;
    for (const auto& entity : scene.entities) order.push_back(&entity);
    switch (scene.camera) {
        case CameraMode::Side:
            std::stable_sort(order.begin(), order.end(), [](const Entity* a, const Entity* b) { return layerOf(*a) < layerOf(*b); });
            break;
        case CameraMode::TopDown:
            std::stable_sort(order.begin(), order.end(), [](const Entity* a, const Entity* b) {
                if (layerOf(*a) != layerOf(*b)) return layerOf(*a) < layerOf(*b);
                return a->worldCollider().bottom() < b->worldCollider().bottom();
            });
            break;
        case CameraMode::Isometric:
            std::stable_sort(order.begin(), order.end(), [](const Entity* a, const Entity* b) {
                return isoDepth(footprintCenter(*a)) < isoDepth(footprintCenter(*b));
            });
            break;
    }
    return order;
}

void GameRenderer::ensureTarget(Vec2 viewSize) {
    const Vec2 rounded{std::max(1.0f, std::ceil(viewSize.x)), std::max(1.0f, std::ceil(viewSize.y))};
    if (target && rounded.x == size.x && rounded.y == size.y) return;
    size = rounded;
    target.reset(SDL_CreateTexture(renderer.native(), SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET,
                                   static_cast<int>(size.x), static_cast<int>(size.y)));
    SDL_SetTextureScaleMode(target.get(), SDL_SCALEMODE_NEAREST);
}

void GameRenderer::drawSideBackdrop(const Backdrop& backdrop, Vec2 camera) {
    renderer.fill({0, 0, size.x, size.y}, backdrop.sky);
    renderer.fill({size.x - moonOffset.x, moonOffset.y, moonSize, moonSize}, backdrop.moon);

    const float offset = std::floor(camera.x * hillParallax);
    const int firstBlock = static_cast<int>(std::floor(offset / hillBlockWidth));
    const float horizon = size.y - camera.y * hillParallax;
    for (int block = firstBlock; block * hillBlockWidth - offset < size.x; ++block) {
        const float height = hillHeight(block);
        renderer.fill({block * hillBlockWidth - offset, horizon - height, hillBlockWidth, height}, backdrop.hills);
    }
}

void GameRenderer::drawSideTiles(const Tilemap& tilemap, Vec2 camera) {
    const CellRange cells = visibleCells(tilemap, camera, size);
    const float tile = Tilemap::tileSize;
    for (int row = cells.firstRow; row <= cells.lastRow; ++row) {
        for (int column = cells.firstColumn; column <= cells.lastColumn; ++column) {
            if (!tilemap.solid(column, row)) continue;
            const char* asset = tilemap.solid(column, row - 1) ? "dirt" : "grass";
            drawSprite(asset, alternating(column, row), Vec2{column * tile, row * tile} - camera);
        }
    }
}

void GameRenderer::drawTopDownTiles(const Tilemap& tilemap, Vec2 camera) {
    const CellRange cells = visibleCells(tilemap, camera, size);
    const float tile = Tilemap::tileSize;
    for (int row = cells.firstRow; row <= cells.lastRow; ++row) {
        for (int column = cells.firstColumn; column <= cells.lastColumn; ++column) {
            const Vec2 position = Vec2{column * tile, row * tile} - camera;
            if (!tilemap.solid(column, row)) {
                drawSprite("floor", alternating(column, row), position);
                continue;
            }
            const bool frontVisible = tilemap.contains(column, row + 1) && !tilemap.solid(column, row + 1);
            drawSprite("wall", frontVisible ? "face" : "idle", position);
        }
    }
}

void GameRenderer::drawIsometric(const Scene& scene, Vec2 camera) {
    const Projection projection = Projection::of(scene);
    const Tilemap& tilemap = scene.tilemap;
    const float tile = Tilemap::tileSize;
    const Rect view{camera.x - isoTileWidth, camera.y - isoTileWidth, size.x + isoTileWidth * 2, size.y + isoTileWidth * 2};

    struct Drawable {
        float depth;
        const Entity* entity;
        Vec2 blockTop;
    };
    std::vector<Drawable> drawables;
    for (int row = 0; row < tilemap.rows(); ++row) {
        for (int column = 0; column < tilemap.columns(); ++column) {
            const Vec2 top = projection.toView({column * tile, row * tile});
            if (!view.contains(top)) continue;
            if (tilemap.solid(column, row)) {
                drawables.push_back({isoDepth({(column + 0.5f) * tile, (row + 0.5f) * tile}), nullptr, top});
            } else {
                drawSprite("iso_floor", alternating(column, row), Vec2{top.x - tile, top.y} - camera);
            }
        }
    }
    for (const Entity* entity : drawOrder(scene)) drawables.push_back({isoDepth(footprintCenter(*entity)), entity, {}});
    std::stable_sort(drawables.begin(), drawables.end(), [](const Drawable& a, const Drawable& b) { return a.depth < b.depth; });

    for (const Drawable& drawable : drawables) {
        if (drawable.entity) {
            drawEntity(*drawable.entity, projection, camera);
        } else {
            drawSprite("iso_block", "idle", Vec2{drawable.blockTop.x - tile, drawable.blockTop.y - isoBlockRise} - camera);
        }
    }
}

void GameRenderer::drawEntity(const Entity& entity, const Projection& projection, Vec2 camera) {
    if (!entity.sprite) return;
    const SpriteAsset* asset = sprites.find(entity.sprite->asset);
    if (!asset) return;
    SDL_Texture* texture = asset->frame(entity.sprite->animation, entity.sprite->animationTime);
    if (!texture) return;
    const Rect bounds = spriteBounds(entity, projection);
    renderer.texture(texture, {std::round(bounds.x - camera.x), std::round(bounds.y - camera.y), bounds.w, bounds.h},
                     entity.sprite->flipX);
}

void GameRenderer::drawSprite(const char* asset, std::string_view animation, Vec2 position) {
    const SpriteAsset* sprite = sprites.find(asset);
    if (!sprite) return;
    SDL_Texture* texture = sprite->frame(std::string(animation), 0);
    if (!texture) return;
    renderer.texture(texture, {std::round(position.x), std::round(position.y), static_cast<float>(sprite->width),
                               static_cast<float>(sprite->height)});
}

void GameRenderer::drawHud(const GameState& state) {
    const Color shadow{0x10, 0x12, 0x1a};
    const Color gold{0xff, 0xc9, 0x4d};
    if (const SpriteAsset* coin = sprites.find("coin")) {
        if (SDL_Texture* texture = coin->preview()) {
            renderer.texture(texture, {8, 8, static_cast<float>(coin->width), static_cast<float>(coin->height)});
        }
    }
    const std::string label = std::format("{}/{}", state.coins, state.totalCoins);
    renderer.pixelText({21, 10}, label, shadow);
    renderer.pixelText({20, 9}, label, gold);
}

}
