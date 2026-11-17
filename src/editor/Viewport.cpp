#include "editor/Viewport.hpp"

#include "game/Gameplay.hpp"

#include <algorithm>
#include <cmath>
#include <format>
#include <vector>

namespace pf {

namespace {

constexpr float minZoom = 1;
constexpr float maxZoom = 8;
constexpr float gizmoLength = 64;
constexpr float handleSize = 10;
constexpr float canvasMargin = 16;
constexpr float snapStep = 8;

Rect handleAt(Vec2 center) { return {center.x - handleSize / 2, center.y - handleSize / 2, handleSize, handleSize}; }

Vec2 center(const Rect& rect) { return {rect.x + rect.w / 2, rect.y + rect.h / 2}; }

float buttonWidth(const Ui& ui, std::string_view text) { return ui.draw.textWidth(text) + 28; }

struct Gizmo {
    Vec2 pivot;
    Vec2 xHandle;
    Vec2 yHandle;
};

Gizmo gizmoFor(const Rect& screenBounds, const Projection& projection) {
    const Vec2 pivot = center(screenBounds);
    return {pivot, pivot + projection.viewDirection({1, 0}) * gizmoLength, pivot + projection.viewDirection({0, -1}) * gizmoLength};
}

std::string_view playHint(const EditorState& state) {
    if (state.mode == PlayMode::Paused) return "PAUSADO";
    if (state.runtime.camera == CameraMode::Side) return "A/D ou setas: mover   Espaço: pular";
    return "WASD ou setas: mover";
}

}

void Viewport::draw(Ui& ui, EditorState& state, GameRenderer& gameRenderer, Rect area) {
    const Theme& theme = ui.theme;
    ui.draw.fill(area, theme.background);
    Rect bar = area.takeTop(theme.toolbarHeight);
    ui.draw.fill(bar, theme.panel);
    ui.draw.line({bar.x, bar.bottom() - 1}, {bar.right(), bar.bottom() - 1}, theme.border);

    const Rect canvas = area.inset(canvasMargin);
    if (state.editing()) frameScene(state, canvas);
    const CanvasMapping mapping = layout(state, canvas);
    drawToolbar(ui, state, bar, mapping);

    Scene& scene = state.active();
    const Vec2 viewSize = state.editing() ? Vec2{canvas.w / mapping.scale, canvas.h / mapping.scale} : gameResolution;
    gameRenderer.render(scene, mapping.camera, viewSize, !state.editing());

    ui.draw.pushClip(canvas);
    const Vec2 drawnSize = gameRenderer.outputSize() * mapping.scale;
    const Rect image{mapping.screen.x, mapping.screen.y, drawnSize.x, drawnSize.y};
    ui.draw.texture(gameRenderer.output(), image);
    drawOverlays(ui, state, gameRenderer, mapping);
    ui.draw.popClip();
    ui.draw.outline(state.editing() ? canvas : image, theme.border);

    handleInput(ui, state, gameRenderer, mapping);
}

void Viewport::frameScene(EditorState& state, const Rect& canvas) {
    const std::pair key{state.scenePath, state.scene.camera};
    if (framedScene == key) return;
    framedScene = key;
    const Vec2 viewSize{canvas.w / zoom, canvas.h / zoom};
    if (const Entity* player = state.scene.findPlayer()) {
        camera = Gameplay::cameraFollowing(state.scene, *player, viewSize);
        return;
    }
    const Rect bounds = Projection::of(state.scene).viewBounds(state.scene.tilemap.bounds());
    camera = Vec2{bounds.x + bounds.w / 2, bounds.y + bounds.h / 2} - viewSize * 0.5f;
}

CanvasMapping Viewport::layout(EditorState& state, const Rect& canvas) const {
    if (state.editing()) return {canvas, zoom, camera};
    const float fit = std::floor(std::min(canvas.w / gameResolution.x, canvas.h / gameResolution.y));
    const float scale = std::max(1.0f, fit);
    const Vec2 size = gameResolution * scale;
    const Rect screen{std::round(canvas.x + (canvas.w - size.x) / 2), std::round(canvas.y + (canvas.h - size.y) / 2), size.x, size.y};
    return {screen, scale, state.runtime.state.camera};
}

void Viewport::drawToolbar(Ui& ui, EditorState& state, Rect bar, const CanvasMapping& mapping) {
    const Theme& theme = ui.theme;
    Rect row = bar.inset(10);
    struct ToolButton {
        Tool tool;
        std::string_view label;
    };
    constexpr ToolButton tools[] = {{Tool::Select, "Selecionar"}, {Tool::Paint, "Pincel"}, {Tool::Erase, "Borracha"}};
    for (const auto& tool : tools) {
        const Rect button = row.takeLeft(buttonWidth(ui, tool.label));
        if (ui.button(button, tool.label, state.tool == tool.tool ? ButtonStyle::Toggled : ButtonStyle::Ghost)) {
            state.tool = tool.tool;
        }
        row.takeLeft(6);
    }

    const std::string info = std::format("{}×{} · {}%", static_cast<int>(gameResolution.x), static_cast<int>(gameResolution.y),
                                         static_cast<int>(mapping.scale * 100));
    ui.labelRight(row, info, theme.textMuted, TextStyle::Mono);
    row.takeRight(ui.draw.textWidth(info, TextStyle::Mono) + 14);

    struct Toggle {
        bool& value;
        std::string_view label;
    };
    Toggle toggles[] = {{state.view.camera, "Câmera"}, {state.view.colliders, "Colisores"}, {state.view.grid, "Grade"}};
    for (auto& toggle : toggles) {
        const Rect button = row.takeRight(buttonWidth(ui, toggle.label));
        if (ui.button(button, toggle.label, toggle.value ? ButtonStyle::Toggled : ButtonStyle::Outlined)) {
            toggle.value = !toggle.value;
        }
        row.takeRight(6);
    }
}

void Viewport::handleInput(Ui& ui, EditorState& state, const GameRenderer& gameRenderer, const CanvasMapping& mapping) {
    Input& input = ui.input;
    const bool hovered = ui.hovered(mapping.screen);
    const Vec2 view = mapping.toView(input.mouse());
    const Vec2 world = Projection::of(state.active()).toWorld(view);

    if (drag == Drag::Pan) {
        camera = camera - input.mouseDelta() * (1 / zoom);
        if (!input.mouseDown(MouseButton::Middle) && !input.mouseDown(MouseButton::Right)) drag = Drag::None;
        return;
    }

    if (hovered && state.editing()) {
        if (input.mousePressed(MouseButton::Middle) || input.mousePressed(MouseButton::Right)) {
            drag = Drag::Pan;
            return;
        }
        if (input.wheel() != 0) {
            zoom = std::clamp(zoom + (input.wheel() > 0 ? 1.0f : -1.0f), minZoom, maxZoom);
            camera = view - (input.mouse() - Vec2{mapping.screen.x, mapping.screen.y}) * (1 / zoom);
        }
    }

    const bool painting = state.editing() && (state.tool == Tool::Paint || state.tool == Tool::Erase);
    if (painting && input.mouseDown(MouseButton::Left) && (hovered || drag == Drag::Free)) {
        if (input.mousePressed(MouseButton::Left) || drag == Drag::Free) {
            drag = Drag::Free;
            paintTile(state, world, state.tool == Tool::Paint);
        }
        return;
    }
    if (painting) {
        drag = Drag::None;
        return;
    }

    if (drag == Drag::None && !(hovered && input.mousePressed(MouseButton::Left))) return;
    handleSelection(ui, state, gameRenderer, mapping);
}

void Viewport::handleSelection(Ui& ui, EditorState& state, const GameRenderer& gameRenderer, const CanvasMapping& mapping) {
    Input& input = ui.input;
    const Projection projection = Projection::of(state.active());
    const Vec2 view = mapping.toView(input.mouse());
    const Vec2 world = projection.toWorld(view);
    Entity* selected = state.selectedEntity();

    if (drag != Drag::None) {
        if (!input.mouseDown(MouseButton::Left) || !selected) {
            drag = Drag::None;
            return;
        }
        Vec2 target = dragStartPosition + (world - dragStartWorld);
        if (input.shift()) target = {std::round(target.x / snapStep) * snapStep, std::round(target.y / snapStep) * snapStep};
        Vec2& position = selected->transform.position;
        const Vec2 next{drag == Drag::AxisY ? position.x : std::round(target.x), drag == Drag::AxisX ? position.y : std::round(target.y)};
        if (next.x != position.x || next.y != position.y) {
            position = next;
            state.modified();
        }
        return;
    }

    if (selected && state.editing()) {
        const Gizmo gizmo = gizmoFor(mapping.toScreen(gameRenderer.spriteBounds(*selected, projection)), projection);
        const Drag axis = handleAt(gizmo.xHandle).contains(input.mouse())   ? Drag::AxisX
                          : handleAt(gizmo.yHandle).contains(input.mouse()) ? Drag::AxisY
                                                                            : Drag::None;
        if (axis != Drag::None) {
            drag = axis;
            dragStartWorld = world;
            dragStartPosition = selected->transform.position;
            return;
        }
    }

    auto order = gameRenderer.drawOrder(state.active());
    std::reverse(order.begin(), order.end());
    for (const Entity* entity : order) {
        if (!gameRenderer.spriteBounds(*entity, projection).contains(view)) continue;
        state.selected = entity->id;
        if (state.editing()) {
            drag = Drag::Free;
            dragStartWorld = world;
            dragStartPosition = entity->transform.position;
        }
        return;
    }
    state.selected.reset();
}

void Viewport::paintTile(EditorState& state, Vec2 world, bool solid) {
    const int column = static_cast<int>(std::floor(world.x / Tilemap::tileSize));
    const int row = static_cast<int>(std::floor(world.y / Tilemap::tileSize));
    Tilemap& tilemap = state.scene.tilemap;
    if (!tilemap.contains(column, row) || tilemap.solid(column, row) == solid) return;
    tilemap.set(column, row, solid);
    state.modified();
}

void Viewport::outlineWorld(Ui& ui, const Projection& projection, const CanvasMapping& mapping, const Rect& world, Color color) {
    std::array<Vec2, 4> corners = projection.outline(world);
    for (Vec2& corner : corners) corner = mapping.toScreen(corner);
    ui.draw.polygon(corners, color);
}

void Viewport::drawOverlays(Ui& ui, EditorState& state, const GameRenderer& gameRenderer, const CanvasMapping& mapping) {
    const Theme& theme = ui.theme;
    Scene& scene = state.active();
    const Projection projection = Projection::of(scene);
    const Rect world = scene.tilemap.bounds();

    if (state.view.grid && state.editing()) {
        const Color gridColor = theme.text.withAlpha(22);
        const float step = Tilemap::tileSize;
        for (float x = world.x; x <= world.right(); x += step) {
            ui.draw.line(mapping.toScreen(projection.toView({x, world.y})), mapping.toScreen(projection.toView({x, world.bottom()})), gridColor);
        }
        for (float y = world.y; y <= world.bottom(); y += step) {
            ui.draw.line(mapping.toScreen(projection.toView({world.x, y})), mapping.toScreen(projection.toView({world.right(), y})), gridColor);
        }
    }
    if (state.editing()) outlineWorld(ui, projection, mapping, world, theme.textMuted.withAlpha(120));

    if (state.view.colliders) {
        for (const auto& entity : scene.entities) {
            if (!entity.body && !entity.pickup) continue;
            const Color color = !entity.body                            ? theme.tagPickup
                                : entity.body->type == BodyType::Static ? theme.tagTerrain
                                                                        : theme.highlight;
            outlineWorld(ui, projection, mapping, entity.worldCollider(), color.withAlpha(200));
        }
    }

    if (state.view.camera && state.editing()) {
        if (const Entity* player = scene.findPlayer()) {
            const Vec2 cameraPosition = Gameplay::cameraFollowing(scene, *player);
            const Rect frame = mapping.toScreen(Rect{cameraPosition.x, cameraPosition.y, gameResolution.x, gameResolution.y});
            ui.draw.dashedOutline(frame, theme.text.withAlpha(150), 4);
            ui.label({frame.x + 6, frame.y - ui.draw.lineHeight() - 4}, "Câmera principal", theme.text.withAlpha(190));
        }
    }

    if (state.tool != Tool::Select && state.editing() && ui.hovered(mapping.screen)) {
        const Vec2 cursor = projection.toWorld(mapping.toView(ui.input.mouse()));
        const float size = Tilemap::tileSize;
        const Rect tile{std::floor(cursor.x / size) * size, std::floor(cursor.y / size) * size, size, size};
        outlineWorld(ui, projection, mapping, tile, state.tool == Tool::Paint ? theme.accent : theme.danger);
    }

    drawSelection(ui, state, gameRenderer, mapping);

    if (!state.editing()) {
        ui.label({mapping.screen.x + 10, mapping.screen.bottom() - ui.draw.lineHeight() - 8}, playHint(state), theme.text.withAlpha(190));
    }
}

void Viewport::drawSelection(Ui& ui, EditorState& state, const GameRenderer& gameRenderer, const CanvasMapping& mapping) {
    const Entity* selected = state.selectedEntity();
    if (!selected) return;
    const Theme& theme = ui.theme;
    const Projection projection = Projection::of(state.active());
    const Rect screenBounds = mapping.toScreen(gameRenderer.spriteBounds(*selected, projection));
    const Rect bounds = screenBounds.inset(-3);
    ui.draw.dashedOutline(bounds, theme.highlight, 3);

    const float tagWidth = ui.draw.textWidth(selected->name) + 12;
    const float tagHeight = ui.draw.lineHeight() + 4;
    const Rect tag{std::round(bounds.x + (bounds.w - tagWidth) / 2), bounds.y - tagHeight - 6, tagWidth, tagHeight};
    ui.draw.fill(tag, theme.highlight);
    ui.labelCentered(tag, selected->name, theme.background);

    if (!state.editing() || state.tool != Tool::Select) return;
    const Gizmo gizmo = gizmoFor(screenBounds, projection);
    for (const float offset : {-0.5f, 0.5f}) {
        ui.draw.line(gizmo.pivot + Vec2{0, offset}, gizmo.xHandle + Vec2{0, offset}, theme.accent);
        ui.draw.line(gizmo.pivot + Vec2{offset, 0}, gizmo.yHandle + Vec2{offset, 0}, theme.highlight);
    }
    ui.draw.fill(handleAt(gizmo.xHandle), theme.accent);
    ui.draw.fill(handleAt(gizmo.yHandle), theme.highlight);
}

}
