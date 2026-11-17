#include "editor/Hierarchy.hpp"

#include "editor/EntityStyle.hpp"

#include <algorithm>
#include <cctype>
#include <format>
#include <set>

namespace pf {

namespace {

constexpr float headerHeight = 52;
constexpr float indentStep = 16;
constexpr float scrollSpeed = 24;

std::string lowercase(std::string_view text) {
    std::string result(text);
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) { return std::tolower(c); });
    return result;
}

Rect textArea(const Rect& row, float indent) { return {row.x + indent, row.y, row.w - indent, row.h}; }

void placeNewEntity(EditorState& state) {
    Entity& entity = state.active().create(std::format("Entidade {}", state.active().nextId()));
    entity.transform.position = {32, 32};
    state.selected = entity.id;
    state.modified();
}

}

void Hierarchy::draw(Ui& ui, EditorState& state, Rect area) {
    const Theme& theme = ui.theme;
    ui.panel(area, theme.panel);

    Rect content = area.inset(theme.padding);
    Rect header = content.takeTop(headerHeight - theme.padding);
    ui.sectionTitle({header.x + 2, header.y + 4, header.w, 28}, "Hierarquia");
    const Rect addButton = header.takeRight(28);
    if (ui.button({addButton.x, addButton.y + 4, 28, 28}, "+", ButtonStyle::Outlined)) placeNewEntity(state);

    content.takeTop(6);
    if (auto typed = ui.textField("hierarchy.filter", content.takeTop(32), filter, "Buscar entidade...")) filter = *typed;
    content.takeTop(10);

    const float rowHeight = theme.rowHeight;
    if (ui.hovered(content)) scroll = std::max(0.0f, scroll - ui.input.wheel() * scrollSpeed);
    ui.draw.pushClip(content);
    Rect rows{content.x - theme.padding + 8, content.y - scroll, content.w + theme.padding * 2 - 16, rowHeight};
    auto nextRow = [&] {
        Rect row = rows;
        rows.y += rowHeight;
        return row;
    };

    Scene& scene = state.active();
    const Rect sceneRow = nextRow();
    if (!state.selected) {
        ui.draw.fill(sceneRow, theme.accentSoft);
    } else if (ui.hovered(sceneRow)) {
        ui.draw.fill(sceneRow, theme.card);
    }
    if (ui.pressed(sceneRow)) state.selected.reset();
    ui.swatch({sceneRow.x + 10, sceneRow.y + rowHeight / 2}, theme.textMuted, 6);
    ui.labelLeft(textArea(sceneRow, 24), scene.name, theme.text);
    ui.labelRight(sceneRow.inset(8), "cena", theme.textMuted, TextStyle::Mono);

    const Rect tilemapRow = nextRow();
    if (ui.hovered(tilemapRow)) ui.draw.fill(tilemapRow, theme.card);
    ui.swatch({tilemapRow.x + 10 + indentStep, tilemapRow.y + rowHeight / 2}, theme.tagTerrain, 6);
    ui.labelLeft(textArea(tilemapRow, 24 + indentStep), "Tilemap", theme.text);
    ui.labelRight(tilemapRow.inset(8), std::format("{}×{}", scene.tilemap.columns(), scene.tilemap.rows()), theme.textMuted, TextStyle::Mono);
    if (ui.pressed(tilemapRow)) {
        state.tool = Tool::Paint;
        state.selected.reset();
    }

    const std::string needle = lowercase(filter);
    if (!needle.empty()) {
        for (const auto& entity : scene.entities) {
            if (lowercase(entity.name).find(needle) != std::string::npos) entityRow(ui, state, entity, nextRow(), indentStep);
        }
    } else {
        std::set<std::string> drawnGroups;
        for (const auto& entity : scene.entities) {
            if (entity.group.empty()) {
                entityRow(ui, state, entity, nextRow(), indentStep);
                continue;
            }
            if (!drawnGroups.insert(entity.group).second) continue;
            const auto count = std::count_if(scene.entities.begin(), scene.entities.end(),
                                             [&](const Entity& other) { return other.group == entity.group; });
            groupRow(ui, entity.group, entityColor(entity, theme), static_cast<int>(count), nextRow());
            for (const auto& member : scene.entities) {
                if (member.group == entity.group) entityRow(ui, state, member, nextRow(), indentStep * 2);
            }
        }
    }
    const float contentHeight = rows.y + scroll - content.y;
    scroll = std::min(scroll, std::max(0.0f, contentHeight - content.h));
    ui.draw.popClip();
}

void Hierarchy::entityRow(Ui& ui, EditorState& state, const Entity& entity, Rect row, float indent) {
    const Theme& theme = ui.theme;
    const bool selected = state.selected == entity.id;
    if (selected) {
        ui.draw.fill(row, theme.accentSoft);
    } else if (ui.hovered(row)) {
        ui.draw.fill(row, theme.card);
    }
    ui.swatch({row.x + 10 + indent, row.y + row.h / 2}, entityColor(entity, theme), 6);
    ui.labelLeft(textArea(row, 24 + indent), entity.name, theme.text);
    if (selected) ui.labelRight(row.inset(8), std::format("{} comp", componentCount(entity)), theme.accent, TextStyle::Mono);
    if (ui.pressed(row)) {
        state.selected = entity.id;
        if (state.tool != Tool::Select) state.tool = Tool::Select;
    }
}

void Hierarchy::groupRow(Ui& ui, const std::string& group, Color color, int count, Rect row) {
    ui.swatch({row.x + 10 + indentStep, row.y + row.h / 2}, color, 6);
    ui.labelLeft(textArea(row, 24 + indentStep), group, ui.theme.text);
    ui.labelRight(row.inset(8), std::to_string(count), ui.theme.textMuted, TextStyle::Mono);
}

}
