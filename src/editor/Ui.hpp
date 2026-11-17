#pragma once

#include "core/Theme.hpp"
#include "platform/Input.hpp"
#include "render/Renderer.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace pf {

enum class ButtonStyle { Ghost, Outlined, Primary, Toggled, Dashed };

class Ui {
public:
    Ui(SDL_Window* window, Renderer& renderer, Input& input, const Theme& theme);

    void beginFrame();
    void endFrame();

    bool hovered(const Rect& rect) const;
    bool pressed(const Rect& rect, MouseButton button = MouseButton::Left) const;
    bool mouseOverPopup() const;

    void panel(const Rect& rect, Color fill);
    void label(Vec2 position, std::string_view text, Color color, TextStyle style = TextStyle::Body);
    void labelLeft(const Rect& rect, std::string_view text, Color color, TextStyle style = TextStyle::Body);
    void labelCentered(const Rect& rect, std::string_view text, Color color, TextStyle style = TextStyle::Body);
    void labelRight(const Rect& rect, std::string_view text, Color color, TextStyle style = TextStyle::Body);
    void sectionTitle(const Rect& rect, std::string_view text);
    void swatch(Vec2 center, Color color, float size = 8);
    bool button(const Rect& rect, std::string_view text, ButtonStyle style = ButtonStyle::Ghost);
    std::optional<std::string> textField(const std::string& id, const Rect& rect, std::string_view value,
                                         std::string_view placeholder = {});
    bool editingText() const { return activeField.has_value(); }

    void beginPopup(const Rect& rect);
    void endPopup();

    Renderer& draw;
    Input& input;
    const Theme& theme;

private:
    float centeredTextY(const Rect& rect, TextStyle style) const;
    void stopEditing();

    SDL_Window* window;
    std::optional<std::string> activeField;
    std::string editBuffer;
    std::optional<std::pair<std::string, std::string>> pendingCommit;
    bool fieldSeenThisFrame = false;
    float caretClock = 0;
    bool insidePopup = false;
    std::optional<Rect> popupThisFrame;
    std::optional<Rect> popupLastFrame;
};

}
