#pragma once

#include "editor/EditorState.hpp"
#include "editor/Ui.hpp"
#include "game/GameRenderer.hpp"

#include <filesystem>
#include <optional>
#include <utility>

namespace pf {

struct CanvasMapping {
    Rect screen;
    float scale = 1;
    Vec2 camera;

    Vec2 toView(Vec2 point) const { return camera + (point - Vec2{screen.x, screen.y}) * (1 / scale); }
    Vec2 toScreen(Vec2 point) const { return Vec2{screen.x, screen.y} + (point - camera) * scale; }
    Rect toScreen(const Rect& rect) const {
        const Vec2 origin = toScreen(Vec2{rect.x, rect.y});
        return {origin.x, origin.y, rect.w * scale, rect.h * scale};
    }
};

class Viewport {
public:
    void draw(Ui& ui, EditorState& state, GameRenderer& gameRenderer, Rect area);

private:
    enum class Drag { None, Free, AxisX, AxisY, Pan };

    void drawToolbar(Ui& ui, EditorState& state, Rect bar, const CanvasMapping& mapping);
    CanvasMapping layout(EditorState& state, const Rect& canvas) const;
    void frameScene(EditorState& state, const Rect& canvas);
    void outlineWorld(Ui& ui, const Projection& projection, const CanvasMapping& mapping, const Rect& world, Color color);
    void handleInput(Ui& ui, EditorState& state, const GameRenderer& gameRenderer, const CanvasMapping& mapping);
    void handleSelection(Ui& ui, EditorState& state, const GameRenderer& gameRenderer, const CanvasMapping& mapping);
    void paintTile(EditorState& state, Vec2 world, bool solid);
    void drawOverlays(Ui& ui, EditorState& state, const GameRenderer& gameRenderer, const CanvasMapping& mapping);
    void drawSelection(Ui& ui, EditorState& state, const GameRenderer& gameRenderer, const CanvasMapping& mapping);

    Vec2 camera{0, 0};
    float zoom = 3;
    Drag drag = Drag::None;
    Vec2 dragStartWorld;
    Vec2 dragStartPosition;
    std::optional<std::pair<std::filesystem::path, CameraMode>> framedScene;
};

}
