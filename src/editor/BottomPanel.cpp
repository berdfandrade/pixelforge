#include "editor/BottomPanel.hpp"

#include "core/Log.hpp"

#include <algorithm>
#include <cmath>
#include <format>

namespace pf {

namespace {

constexpr float tabHeight = 40;
constexpr float cardWidth = 118;
constexpr float cardHeight = 66;
constexpr float cardGap = 11;
constexpr float thumbnailHeight = 36;
constexpr float consoleLineHeight = 18;

Color levelColor(LogLevel level, const Theme& theme) {
    switch (level) {
        case LogLevel::Warning: return theme.warning;
        case LogLevel::Error: return theme.danger;
        case LogLevel::Info: break;
    }
    return theme.text;
}

void assignSprite(EditorState& state, const SpriteAsset& asset) {
    Entity* entity = state.selectedEntity();
    if (!entity) return;
    if (!entity->sprite) entity->sprite = Sprite{};
    entity->sprite->asset = asset.name;
    state.modified();
}

}

void BottomPanel::draw(Ui& ui, EditorState& state, Rect area) {
    const Theme& theme = ui.theme;
    ui.panel(area, theme.panel);
    Rect tabs = area.takeTop(tabHeight);
    ui.draw.line({tabs.x, tabs.bottom() - 1}, {tabs.right(), tabs.bottom() - 1}, theme.border);
    tabs.takeLeft(theme.padding);

    struct Tab {
        BottomTab tab;
        std::string_view label;
    };
    constexpr Tab entries[] = {{BottomTab::Assets, "Assets"}, {BottomTab::Console, "Console"}};
    for (const auto& entry : entries) {
        const Rect tab = tabs.takeLeft(ui.draw.textWidth(entry.label) + 28);
        const bool active = state.bottomTab == entry.tab;
        if (ui.hovered(tab) && !active) ui.draw.fill(tab, theme.card);
        ui.labelCentered(tab, entry.label, active ? theme.text : theme.textMuted);
        if (active) ui.draw.fill({tab.x, tab.bottom() - 2, tab.w, 2}, theme.accent);
        if (ui.pressed(tab)) state.bottomTab = entry.tab;
    }

    const Rect content = area.inset(theme.padding);
    if (state.bottomTab == BottomTab::Assets) {
        drawAssets(ui, state, content);
    } else {
        drawConsole(ui, content);
    }
}

void BottomPanel::drawAssets(Ui& ui, EditorState& state, Rect area) {
    const Theme& theme = ui.theme;
    const auto& sprites = state.sprites.all();
    const float contentWidth = static_cast<float>(sprites.size()) * (cardWidth + cardGap);
    if (ui.hovered(area)) assetScroll -= ui.input.wheel() * 40;
    assetScroll = std::clamp(assetScroll, 0.0f, std::max(0.0f, contentWidth - area.w));

    ui.draw.pushClip(area);
    float x = area.x - assetScroll;
    for (const auto& asset : sprites) {
        const Rect card{x, area.y, cardWidth, cardHeight};
        const bool isHovered = ui.hovered(card);
        ui.draw.fill(card, isHovered ? theme.card : theme.field);
        ui.draw.outline(card, isHovered ? theme.textMuted : theme.border);
        if (SDL_Texture* preview = asset.preview()) {
            const float assetWidth = static_cast<float>(asset.width);
            const float assetHeight = static_cast<float>(asset.height);
            const float scale = std::max(1.0f, std::floor(std::min(thumbnailHeight / assetHeight, (cardWidth - 16) / assetWidth)));
            const float width = assetWidth * scale;
            const float height = assetHeight * scale;
            ui.draw.texture(preview, {std::round(card.x + (card.w - width) / 2), std::round(card.y + (card.h - height) / 2), width, height});
        }
        ui.draw.pushClip({std::max(x, area.x), card.bottom(), std::min(cardWidth, area.right() - x), area.bottom() - card.bottom()});
        ui.label({x, card.bottom() + 8}, asset.name + ".sprite", theme.text, TextStyle::Mono);
        ui.label({x, card.bottom() + 26}, std::format("Sprite · {} anim", asset.animations.size()), theme.textMuted);
        ui.draw.popClip();
        if (ui.pressed(card)) assignSprite(state, asset);
        x += cardWidth + cardGap;
    }
    ui.draw.popClip();
}

void BottomPanel::drawConsole(Ui& ui, Rect area) {
    const auto& entries = Log::entries();
    const auto visible = static_cast<std::size_t>(std::max(1.0f, std::floor(area.h / consoleLineHeight)));
    const std::size_t first = entries.size() > visible ? entries.size() - visible : 0;
    ui.draw.pushClip(area);
    float y = area.y;
    for (std::size_t index = first; index < entries.size(); ++index) {
        ui.label({area.x, y}, entries[index].message, levelColor(entries[index].level, ui.theme), TextStyle::Mono);
        y += consoleLineHeight;
    }
    ui.draw.popClip();
}

}
