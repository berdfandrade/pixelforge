#include "App.hpp"

#include "core/Log.hpp"
#include "editor/Editor.hpp"
#include "game/GameRenderer.hpp"
#include "game/Gameplay.hpp"
#include "scene/SceneSerializer.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace pf {

namespace {

constexpr int windowWidth = 1440;
constexpr int windowHeight = 900;
constexpr float fpsSmoothing = 0.05f;
constexpr float maxFrameSeconds = 0.25f;
constexpr int screenshotFrame = 10;

class FrameClock {
public:
    float tick() {
        const Uint64 now = SDL_GetTicksNS();
        const float delta = previous == 0 ? 0 : static_cast<float>(now - previous) / 1e9f;
        previous = now;
        if (delta > 0) framesPerSecond += (1 / delta - framesPerSecond) * fpsSmoothing;
        return delta;
    }

    float fps() const { return framesPerSecond; }

private:
    Uint64 previous = 0;
    float framesPerSecond = 60;
};

}

App::App(AppOptions options) : options(std::move(options)) {
    if (!SDL_Init(SDL_INIT_VIDEO)) throw std::runtime_error(std::string("SDL_Init: ") + SDL_GetError());
    const SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    if (!SDL_CreateWindowAndRenderer("PixelForge", windowWidth, windowHeight, flags, &window, &sdlRenderer)) {
        throw std::runtime_error(std::string("SDL_CreateWindowAndRenderer: ") + SDL_GetError());
    }
    SDL_SetWindowMinimumSize(window, 1100, 680);
    SDL_SetRenderVSync(sdlRenderer, 1);

    const auto themePath = this->options.assets / "theme.css";
    const auto spritesPath = this->options.assets / "sprites";
    theme = Theme::load(themePath);
    renderer = std::make_unique<Renderer>(window, sdlRenderer, this->options.assets / "fonts");
    renderer->setFontSizes(theme.fontSize, theme.monoSize);
    sprites = std::make_unique<SpriteLibrary>(sdlRenderer, spritesPath);
    themeWatcher = std::make_unique<FileWatcher>(themePath);
    spriteWatcher = std::make_unique<FileWatcher>(spritesPath);
}

App::~App() {
    sprites.reset();
    renderer.reset();
    SDL_DestroyRenderer(sdlRenderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

int App::run() {
    auto scene = SceneSerializer::load(options.scene);
    if (!scene) return 1;
    Log::info("Cena carregada: " + options.scene.filename().string());
    return options.playOnly ? runGame(std::move(*scene)) : runEditor(std::move(*scene));
}

int App::runEditor(Scene scene) {
    Editor editor(window, *renderer, input, theme, *sprites, std::move(scene), options.scene);
    FrameClock clock;
    for (int frame = 1;; ++frame) {
        const float delta = clock.tick();
        const bool windowClosed = !pumpEvents();
        if ((windowClosed || editor.quitRequested()) && editor.confirmDiscardChanges()) break;
        if (windowClosed) input.cancelQuit();

        hotReload(delta);
        editor.update(delta);
        renderer->beginFrame(theme.background);
        editor.draw(clock.fps());
        if (options.screenshot && frame == screenshotFrame) {
            if (!renderer->capture(*options.screenshot)) Log::error(std::string("Falha ao salvar screenshot: ") + SDL_GetError());
            break;
        }
        renderer->endFrame();
    }
    return 0;
}

int App::runGame(Scene scene) {
    GameRenderer gameRenderer(*renderer, *sprites);
    Scene runtime = scene;
    Gameplay::begin(runtime);
    FrameClock clock;
    float accumulator = 0;
    for (int frame = 1; pumpEvents(); ++frame) {
        const float delta = clock.tick();
        bool jumpPressed = GameInput::jumpKeyPressed(input);
        accumulator += std::min(delta, maxFrameSeconds);
        while (accumulator >= Gameplay::tickSeconds) {
            Gameplay::tick(runtime, GameInput::read(input, jumpPressed));
            jumpPressed = false;
            accumulator -= Gameplay::tickSeconds;
            if (runtime.state.defeated) {
                runtime = scene;
                Gameplay::begin(runtime);
            }
        }
        Gameplay::animate(runtime, delta);

        renderer->beginFrame({0, 0, 0});
        gameRenderer.render(runtime, runtime.state.camera, gameResolution, true);
        const Vec2 output = renderer->outputSize();
        const float scale = std::max(1.0f, std::floor(std::min(output.x / gameResolution.x, output.y / gameResolution.y)));
        const Vec2 size = gameResolution * scale;
        renderer->texture(gameRenderer.output(), {std::round((output.x - size.x) / 2), std::round((output.y - size.y) / 2), size.x, size.y});
        if (options.screenshot && frame == screenshotFrame) {
            if (!renderer->capture(*options.screenshot)) Log::error(std::string("Falha ao salvar screenshot: ") + SDL_GetError());
            break;
        }
        renderer->endFrame();
    }
    return 0;
}

bool App::pumpEvents() {
    input.beginFrame();
    SDL_Event event;
    while (SDL_PollEvent(&event)) input.handle(event);
    return !input.quitRequested();
}

void App::hotReload(float deltaSeconds) {
    if (themeWatcher->poll(deltaSeconds)) {
        theme = Theme::load(options.assets / "theme.css");
        renderer->setFontSizes(theme.fontSize, theme.monoSize);
        Log::info("Hot reload: theme.css");
    }
    if (spriteWatcher->poll(deltaSeconds)) {
        sprites->reload();
        Log::info("Hot reload: sprites");
    }
}

}
