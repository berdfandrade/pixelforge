#pragma once

#include "core/FileWatcher.hpp"
#include "core/Theme.hpp"
#include "platform/Input.hpp"
#include "render/Renderer.hpp"
#include "render/SpriteLibrary.hpp"
#include "scene/Scene.hpp"

#include <SDL3/SDL.h>

#include <filesystem>
#include <memory>
#include <optional>

namespace pf {

struct AppOptions {
    std::filesystem::path assets;
    std::filesystem::path scene;
    bool playOnly = false;
    std::optional<std::filesystem::path> screenshot;
};

class App {
public:
    explicit App(AppOptions options);
    ~App();

    App(const App&) = delete;
    App& operator=(const App&) = delete;

    int run();

private:
    int runEditor(Scene scene);
    int runGame(Scene scene);
    bool pumpEvents();
    void hotReload(float deltaSeconds);

    AppOptions options;
    SDL_Window* window = nullptr;
    SDL_Renderer* sdlRenderer = nullptr;
    std::unique_ptr<Renderer> renderer;
    std::unique_ptr<SpriteLibrary> sprites;
    std::unique_ptr<FileWatcher> themeWatcher;
    std::unique_ptr<FileWatcher> spriteWatcher;
    Theme theme;
    Input input;
};

}
