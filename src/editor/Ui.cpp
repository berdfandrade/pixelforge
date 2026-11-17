#include "editor/Ui.hpp"

#include <cmath>

namespace pf {

namespace {

constexpr float caretBlinkSeconds = 0.5f;

void popCodepoint(std::string& text) {
    while (!text.empty() && (static_cast<unsigned char>(text.back()) & 0xC0) == 0x80) text.pop_back();
    if (!text.empty()) text.pop_back();
}

}

Ui::Ui(SDL_Window* window, Renderer& renderer, Input& input, const Theme& theme)
    : draw(renderer), input(input), theme(theme), window(window) {}

void Ui::beginFrame() {
    fieldSeenThisFrame = false;
    popupThisFrame.reset();
    caretClock += 1.0f / 60.0f;
}

void Ui::endFrame() {
    if (activeField && !fieldSeenThisFrame) stopEditing();
    popupLastFrame = popupThisFrame;
}

bool Ui::hovered(const Rect& rect) const {
    if (!insidePopup && mouseOverPopup()) return false;
    return rect.contains(input.mouse());
}

bool Ui::pressed(const Rect& rect, MouseButton button) const { return input.mousePressed(button) && hovered(rect); }

bool Ui::mouseOverPopup() const { return popupLastFrame && popupLastFrame->contains(input.mouse()); }

void Ui::panel(const Rect& rect, Color fill) {
    draw.fill(rect, fill);
    draw.outline(rect, theme.border);
}

float Ui::centeredTextY(const Rect& rect, TextStyle style) const {
    return std::round(rect.y + (rect.h - draw.lineHeight(style)) / 2);
}

void Ui::label(Vec2 position, std::string_view text, Color color, TextStyle style) { draw.text(position, text, color, style); }

void Ui::labelLeft(const Rect& rect, std::string_view text, Color color, TextStyle style) {
    draw.text({rect.x, centeredTextY(rect, style)}, text, color, style);
}

void Ui::labelCentered(const Rect& rect, std::string_view text, Color color, TextStyle style) {
    const float x = std::round(rect.x + (rect.w - draw.textWidth(text, style)) / 2);
    draw.text({x, centeredTextY(rect, style)}, text, color, style);
}

void Ui::labelRight(const Rect& rect, std::string_view text, Color color, TextStyle style) {
    draw.text({rect.right() - draw.textWidth(text, style), centeredTextY(rect, style)}, text, color, style);
}

void Ui::sectionTitle(const Rect& rect, std::string_view text) { labelLeft(rect, text, theme.textMuted, TextStyle::Strong); }

void Ui::swatch(Vec2 center, Color color, float size) {
    draw.fill({std::round(center.x - size / 2), std::round(center.y - size / 2), size, size}, color);
}

bool Ui::button(const Rect& rect, std::string_view text, ButtonStyle style) {
    const bool isHovered = hovered(rect);
    Color textColor = theme.text;
    switch (style) {
        case ButtonStyle::Ghost:
            if (isHovered) draw.fill(rect, theme.card);
            break;
        case ButtonStyle::Outlined:
            draw.fill(rect, isHovered ? theme.card : theme.panel);
            draw.outline(rect, theme.border);
            break;
        case ButtonStyle::Primary:
            draw.fill(rect, theme.accent);
            if (isHovered) draw.outline(rect, theme.text);
            textColor = theme.background;
            break;
        case ButtonStyle::Toggled:
            draw.fill(rect, theme.card);
            draw.outline(rect, theme.highlight);
            textColor = theme.highlight;
            break;
        case ButtonStyle::Dashed:
            if (isHovered) draw.fill(rect, theme.card);
            draw.dashedOutline(rect, theme.border, 4);
            break;
    }
    if (!text.empty()) labelCentered(rect, text, textColor);
    return pressed(rect);
}

std::optional<std::string> Ui::textField(const std::string& id, const Rect& rect, std::string_view value,
                                         std::string_view placeholder) {
    const bool active = activeField == id;
    const bool isHovered = hovered(rect);
    draw.fill(rect, theme.field);
    draw.outline(rect, active ? theme.accent : isHovered ? theme.textMuted : theme.border);
    const Vec2 textPosition{rect.x + 8, centeredTextY(rect, TextStyle::Mono)};
    draw.pushClip(rect.inset(2));

    std::optional<std::string> committed;
    if (pendingCommit && pendingCommit->first == id) {
        committed = std::move(pendingCommit->second);
        pendingCommit.reset();
    }
    if (active) {
        fieldSeenThisFrame = true;
        editBuffer += input.typedText();
        if (input.keyPressed(SDL_SCANCODE_BACKSPACE)) popCodepoint(editBuffer);
        const bool clickedAway = input.mousePressed(MouseButton::Left) && !rect.contains(input.mouse());
        if (input.keyPressed(SDL_SCANCODE_RETURN) || input.keyPressed(SDL_SCANCODE_KP_ENTER) || clickedAway) {
            committed = editBuffer;
            stopEditing();
        } else if (input.keyPressed(SDL_SCANCODE_ESCAPE)) {
            stopEditing();
        }
        draw.text(textPosition, editBuffer, theme.text, TextStyle::Mono);
        if (activeField && std::fmod(caretClock, caretBlinkSeconds * 2) < caretBlinkSeconds) {
            const float caretX = textPosition.x + draw.textWidth(editBuffer, TextStyle::Mono) + 1;
            draw.fill({caretX, textPosition.y, 1, draw.lineHeight(TextStyle::Mono)}, theme.accent);
        }
    } else {
        if (value.empty()) {
            draw.text(textPosition, placeholder, theme.textMuted, TextStyle::Mono);
        } else {
            draw.text(textPosition, value, theme.text, TextStyle::Mono);
        }
        if (pressed(rect)) {
            if (activeField) pendingCommit = std::pair{*activeField, editBuffer};
            activeField = id;
            editBuffer = std::string(value);
            fieldSeenThisFrame = true;
            caretClock = 0;
            SDL_StartTextInput(window);
        }
    }
    draw.popClip();
    return committed;
}

void Ui::beginPopup(const Rect& rect) {
    insidePopup = true;
    popupThisFrame = rect;
    draw.fill(rect, theme.card);
    draw.outline(rect, theme.border);
}

void Ui::endPopup() { insidePopup = false; }

void Ui::stopEditing() {
    activeField.reset();
    editBuffer.clear();
    SDL_StopTextInput(window);
}

}
