#pragma once

#include "editor/BottomPanel.hpp"
#include "editor/EditorState.hpp"
#include "editor/Hierarchy.hpp"
#include "editor/Inspector.hpp"
#include "editor/Ui.hpp"
#include "editor/Viewport.hpp"
#include "game/GameRenderer.hpp"

#include <filesystem>
#include <optional>

namespace pf {

class Editor {
public:
    Editor(SDL_Window* window, Renderer& renderer, Input& input, const Theme& theme, const SpriteLibrary& sprites,
           Scene scene, std::filesystem::path scenePath);

    void update(float deltaSeconds);
    void draw(float framesPerSecond);
    bool quitRequested() const { return quitting; }
    bool confirmDiscardChanges();

private:
    void handleShortcuts();
    void simulate(float deltaSeconds);
    void tick(bool jumpPressed);

    void save();
    void reloadScene();
    void openScene(const std::filesystem::path& path);
    void togglePlay();
    void togglePause();
    void requestStep();
    void duplicateSelected();
    void deleteSelected();
    void showShortcuts();
    void requestQuit() { quitting = true; }

    void drawTopBar(Rect bar);
    void drawLogo(Vec2 origin);
    void drawMenus(Rect bar);
    void drawPlayControls(Rect group);
    void drawStatusBar(Rect bar, float framesPerSecond);

    SDL_Window* window;
    Ui ui;
    GameRenderer gameRenderer;
    EditorState state;
    Hierarchy hierarchy;
    Inspector inspector;
    Viewport viewport;
    BottomPanel bottomPanel;
    std::optional<std::size_t> openMenu;
    float accumulator = 0;
    bool stepRequested = false;
    float physicsMilliseconds = 0;
    bool quitting = false;
};

}
