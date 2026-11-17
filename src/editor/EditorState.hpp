#pragma once

#include "game/GameRenderer.hpp"
#include "render/SpriteLibrary.hpp"
#include "scene/Scene.hpp"

#include <filesystem>
#include <optional>

namespace pf {

enum class PlayMode { Editing, Playing, Paused };
enum class Tool { Select, Paint, Erase };
enum class BottomTab { Assets, Console };

struct ViewOptions {
    bool grid = true;
    bool colliders = true;
    bool camera = true;
};

struct EditorState {
    Scene scene;
    Scene runtime;
    std::filesystem::path scenePath;
    PlayMode mode = PlayMode::Editing;
    bool dirty = false;
    std::optional<int> selected;
    Tool tool = Tool::Select;
    ViewOptions view;
    BottomTab bottomTab = BottomTab::Assets;
    const SpriteLibrary& sprites;
    const GameRenderer& gameRenderer;

    bool editing() const { return mode == PlayMode::Editing; }
    Scene& active() { return editing() ? scene : runtime; }
    Entity* selectedEntity() { return selected ? active().find(*selected) : nullptr; }

    void modified() {
        if (editing()) dirty = true;
    }
};

}
