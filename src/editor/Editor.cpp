#include "editor/Editor.hpp"

#include "core/Log.hpp"
#include "game/Gameplay.hpp"
#include "scene/SceneSerializer.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <format>
#include <functional>
#include <vector>

namespace pf {

namespace {

constexpr float maxFrameSeconds = 0.25f;
constexpr float menuItemHeight = 28;
constexpr float menuWidth = 230;
constexpr float playButtonWidth = 46;

struct MenuItem {
    std::string label;
    std::string_view shortcut;
    std::function<void()> action;
};

struct Menu {
    std::string_view title;
    std::vector<MenuItem> items;
};

std::vector<std::filesystem::path> scenesBeside(const std::filesystem::path& scenePath) {
    std::vector<std::filesystem::path> scenes;
    std::error_code error;
    for (const auto& entry : std::filesystem::directory_iterator(scenePath.parent_path(), error)) {
        if (entry.path().extension() == ".scene") scenes.push_back(entry.path());
    }
    std::sort(scenes.begin(), scenes.end());
    return scenes;
}

}

Editor::Editor(SDL_Window* window, Renderer& renderer, Input& input, const Theme& theme, const SpriteLibrary& sprites,
               Scene scene, std::filesystem::path scenePath)
    : window(window),
      ui(window, renderer, input, theme),
      gameRenderer(renderer, sprites),
      state{std::move(scene), {}, std::move(scenePath), PlayMode::Editing, false, std::nullopt, Tool::Select, {},
            BottomTab::Assets, sprites, gameRenderer} {
    if (Entity* player = state.scene.findPlayer()) state.selected = player->id;
}

void Editor::update(float deltaSeconds) {
    handleShortcuts();
    simulate(deltaSeconds);
}

void Editor::handleShortcuts() {
    const Input& input = ui.input;
    if (ui.editingText()) return;
    if (input.ctrl() && input.keyPressed(SDL_SCANCODE_S)) save();
    if (input.ctrl() && input.keyPressed(SDL_SCANCODE_D)) duplicateSelected();
    if (input.keyPressed(SDL_SCANCODE_DELETE)) deleteSelected();
    if (input.keyPressed(SDL_SCANCODE_F5)) togglePlay();
    if (input.keyPressed(SDL_SCANCODE_F6)) togglePause();
    if (input.keyPressed(SDL_SCANCODE_F7)) requestStep();
    if (input.keyPressed(SDL_SCANCODE_ESCAPE)) {
        if (openMenu) {
            openMenu.reset();
        } else {
            state.selected.reset();
        }
    }
    if (!state.editing() || input.ctrl()) return;
    if (input.keyPressed(SDL_SCANCODE_V)) state.tool = Tool::Select;
    if (input.keyPressed(SDL_SCANCODE_B)) state.tool = Tool::Paint;
    if (input.keyPressed(SDL_SCANCODE_E)) state.tool = Tool::Erase;
    if (input.keyPressed(SDL_SCANCODE_G)) state.view.grid = !state.view.grid;
}

void Editor::simulate(float deltaSeconds) {
    Gameplay::animate(state.active(), state.mode == PlayMode::Paused ? 0 : deltaSeconds);
    if (state.mode == PlayMode::Paused && stepRequested) tick(false);
    stepRequested = false;
    if (state.mode != PlayMode::Playing) return;

    bool jumpPressed = !ui.editingText() && GameInput::jumpKeyPressed(ui.input);
    accumulator += std::min(deltaSeconds, maxFrameSeconds);
    while (accumulator >= Gameplay::tickSeconds) {
        tick(jumpPressed);
        jumpPressed = false;
        accumulator -= Gameplay::tickSeconds;
    }
}

void Editor::tick(bool jumpPressed) {
    const GameInput gameInput = ui.editingText() ? GameInput{} : GameInput::read(ui.input, jumpPressed);
    const Uint64 start = SDL_GetPerformanceCounter();
    Gameplay::tick(state.runtime, gameInput);
    physicsMilliseconds = static_cast<float>(SDL_GetPerformanceCounter() - start) * 1000.0f /
                          static_cast<float>(SDL_GetPerformanceFrequency());

    if (state.runtime.state.defeated) {
        Log::warning("Player derrotado. Reiniciando a cena.");
        state.runtime = state.scene;
        Gameplay::begin(state.runtime);
    }
}

void Editor::save() {
    if (SceneSerializer::save(state.scene, state.scenePath)) {
        state.dirty = false;
        Log::info("Cena salva em " + state.scenePath.filename().string());
    }
}

void Editor::reloadScene() { openScene(state.scenePath); }

void Editor::openScene(const std::filesystem::path& path) {
    if (!confirmDiscardChanges()) return;
    auto loaded = SceneSerializer::load(path);
    if (!loaded) return;
    state.mode = PlayMode::Editing;
    state.scene = std::move(*loaded);
    state.scenePath = path;
    state.dirty = false;
    state.selected.reset();
    if (const Entity* player = state.scene.findPlayer()) state.selected = player->id;
    Log::info("Cena aberta: " + path.filename().string());
}

void Editor::togglePlay() {
    if (state.editing()) {
        state.runtime = state.scene;
        Gameplay::begin(state.runtime);
        state.mode = PlayMode::Playing;
        accumulator = 0;
        Log::info("Play: simulando cópia da cena.");
    } else {
        state.mode = PlayMode::Editing;
        Log::info("Stop: cena restaurada.");
    }
}

void Editor::togglePause() {
    if (state.mode == PlayMode::Playing) {
        state.mode = PlayMode::Paused;
    } else if (state.mode == PlayMode::Paused) {
        state.mode = PlayMode::Playing;
    }
}

void Editor::requestStep() {
    if (state.editing()) {
        togglePlay();
        state.mode = PlayMode::Paused;
    }
    stepRequested = true;
}

void Editor::duplicateSelected() {
    const Entity* selected = state.selectedEntity();
    if (!selected) return;
    Scene& scene = state.active();
    Entity copy = *selected;
    copy.id = scene.nextId();
    copy.name += " (cópia)";
    copy.transform.position.x += Tilemap::tileSize;
    const auto position = std::find_if(scene.entities.begin(), scene.entities.end(),
                                       [&](const Entity& entity) { return entity.id == selected->id; });
    scene.entities.insert(std::next(position), copy);
    state.selected = copy.id;
    state.modified();
}

void Editor::deleteSelected() {
    if (!state.selected) return;
    state.active().remove(*state.selected);
    state.selected.reset();
    state.modified();
}

void Editor::showShortcuts() {
    Log::info("Atalhos: Ctrl+S salvar · Ctrl+D duplicar · Del excluir · F5 play/stop · F6 pausa · F7 avançar quadro");
    Log::info("Ferramentas: V selecionar · B pincel · E borracha · G grade · Shift arrasta em passos de 8px");
    Log::info("Viewport: roda do mouse zoom · botão direito/meio arrasta a câmera");
    state.bottomTab = BottomTab::Console;
}

bool Editor::confirmDiscardChanges() {
    if (!state.dirty) return true;
    const SDL_MessageBoxButtonData buttons[] = {
        {SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 0, "Salvar"},
        {0, 1, "Descartar"},
        {SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 2, "Cancelar"},
    };
    const std::string message = state.scenePath.filename().string() + " tem alterações não salvas.";
    const SDL_MessageBoxData data{SDL_MESSAGEBOX_WARNING, window, "PixelForge", message.c_str(), 3, buttons, nullptr};
    int choice = 2;
    if (!SDL_ShowMessageBox(&data, &choice)) return false;
    if (choice == 0) save();
    if (choice == 2) quitting = false;
    return choice != 2 && !state.dirty;
}

void Editor::draw(float framesPerSecond) {
    const Theme& theme = ui.theme;
    ui.beginFrame();
    const Vec2 size = ui.draw.outputSize();
    Rect screen{0, 0, size.x, size.y};

    const Rect header = screen.takeTop(theme.headerHeight);
    const Rect status = screen.takeBottom(theme.statusHeight);
    const Rect left = screen.takeLeft(theme.sidebarWidth);
    const Rect right = screen.takeRight(theme.inspectorWidth);
    const Rect bottom = screen.takeBottom(theme.bottomHeight);

    drawTopBar(header);
    hierarchy.draw(ui, state, left);
    viewport.draw(ui, state, gameRenderer, screen);
    bottomPanel.draw(ui, state, bottom);
    drawStatusBar(status, framesPerSecond);
    inspector.draw(ui, state, right);
    drawMenus(header);
    ui.endFrame();
}

void Editor::drawTopBar(Rect bar) {
    const Theme& theme = ui.theme;
    ui.draw.fill(bar, theme.panel);
    ui.draw.line({bar.x, bar.bottom() - 1}, {bar.right(), bar.bottom() - 1}, theme.border);
    drawLogo({bar.x + 22, bar.y + 26});

    const Rect group{std::round(bar.x + (bar.w - playButtonWidth * 3 - 12) / 2), bar.y + 9, playButtonWidth * 3 + 12, bar.h - 18};
    drawPlayControls(group);

    Rect right = bar.inset(12);
    if (ui.button(right.takeRight(86), "Salvar", ButtonStyle::Outlined)) save();
    right.takeRight(14);
    const std::string fileStatus = std::format("{} · {}", state.scenePath.filename().string(), state.dirty ? "modificado" : "salvo");
    ui.labelRight(right, fileStatus, state.dirty ? theme.warning : theme.textMuted, TextStyle::Mono);
}

void Editor::drawLogo(Vec2 origin) {
    const Theme& theme = ui.theme;
    ui.draw.fill({origin.x - 6, origin.y + 1, 6, 3}, theme.text);
    ui.draw.fill({origin.x, origin.y, 26, 5}, theme.text);
    ui.draw.fill({origin.x + 8, origin.y + 5, 10, 5}, theme.textMuted);
    ui.draw.fill({origin.x + 3, origin.y + 10, 20, 4}, theme.text);
    ui.draw.fill({origin.x + 21, origin.y - 6, 2, 2}, theme.accent);
    ui.draw.fill({origin.x + 25, origin.y - 9, 2, 2}, theme.danger);
    ui.draw.pixelText({origin.x + 40, origin.y - 1}, "PIXELFORGE", theme.text, 2);
}

void Editor::drawPlayControls(Rect group) {
    const Theme& theme = ui.theme;
    ui.draw.fill(group, theme.background);
    ui.draw.outline(group, theme.border);
    Rect row = group.inset(4);

    const Rect playButton = row.takeLeft(playButtonWidth);
    if (ui.button(playButton, {}, state.editing() ? ButtonStyle::Primary : ButtonStyle::Toggled)) togglePlay();
    const Vec2 playCenter{playButton.x + playButton.w / 2, playButton.y + playButton.h / 2};
    if (state.editing()) {
        for (int column = 0; column < 9; ++column) {
            const float height = 12 - static_cast<float>(column) * 1.4f;
            ui.draw.fill({playCenter.x - 4 + column, std::round(playCenter.y - height / 2), 1, std::round(height)}, theme.background);
        }
    } else {
        ui.draw.fill({playCenter.x - 5, playCenter.y - 5, 10, 10}, theme.highlight);
    }

    row.takeLeft(2);
    const Rect pauseButton = row.takeLeft(playButtonWidth);
    if (ui.button(pauseButton, {}, state.mode == PlayMode::Paused ? ButtonStyle::Toggled : ButtonStyle::Ghost)) togglePause();
    const Vec2 pauseCenter{pauseButton.x + pauseButton.w / 2, pauseButton.y + pauseButton.h / 2};
    const Color pauseColor = state.editing() ? theme.textMuted : theme.text;
    ui.draw.fill({pauseCenter.x - 5, pauseCenter.y - 5, 3, 11}, pauseColor);
    ui.draw.fill({pauseCenter.x + 2, pauseCenter.y - 5, 3, 11}, pauseColor);

    row.takeLeft(2);
    const Rect stepButton = row.takeLeft(playButtonWidth);
    if (ui.button(stepButton, {}, ButtonStyle::Ghost)) requestStep();
    const Vec2 stepCenter{stepButton.x + stepButton.w / 2, stepButton.y + stepButton.h / 2};
    for (int column = 0; column < 6; ++column) {
        const float height = 10 - static_cast<float>(column) * 1.6f;
        ui.draw.fill({stepCenter.x - 5 + column, std::round(stepCenter.y - height / 2), 1, std::round(height)}, theme.text);
    }
    ui.draw.fill({stepCenter.x + 3, stepCenter.y - 5, 2, 10}, theme.text);
}

void Editor::drawMenus(Rect bar) {
    std::vector<MenuItem> fileItems{
        {"Salvar cena", "Ctrl+S", [this] { save(); }},
        {"Recarregar do disco", "", [this] { reloadScene(); }},
    };
    if (openMenu == 0) {
        for (const auto& path : scenesBeside(state.scenePath)) {
            if (path != state.scenePath) fileItems.push_back({"Abrir " + path.filename().string(), "", [this, path] { openScene(path); }});
        }
    }
    fileItems.push_back({"Sair", "", [this] { requestQuit(); }});
    const std::array<Menu, 4> menus{{
        {"Arquivo", std::move(fileItems)},
        {"Editar",
         {{"Duplicar entidade", "Ctrl+D", [this] { duplicateSelected(); }},
          {"Excluir entidade", "Del", [this] { deleteSelected(); }}}},
        {"Cena",
         {{"Play / Stop", "F5", [this] { togglePlay(); }},
          {"Pausar / Continuar", "F6", [this] { togglePause(); }},
          {"Avançar um quadro", "F7", [this] { requestStep(); }}}},
        {"Ajuda", {{"Atalhos", "", [this] { showShortcuts(); }}}},
    }};

    const Theme& theme = ui.theme;
    float x = bar.x + 228;
    std::array<Rect, menus.size()> titles;
    for (std::size_t index = 0; index < menus.size(); ++index) {
        titles[index] = {x, bar.y + 14, ui.draw.textWidth(menus[index].title) + 24, bar.h - 28};
        const bool open = openMenu == index;
        if (open || ui.hovered(titles[index])) ui.draw.fill(titles[index], theme.card);
        ui.labelCentered(titles[index], menus[index].title, theme.text);
        if (ui.pressed(titles[index])) {
            openMenu = open ? std::nullopt : std::optional{index};
            return;
        }
        if (openMenu && !open && ui.hovered(titles[index])) openMenu = index;
        x += titles[index].w + 4;
    }
    if (!openMenu) return;

    const Menu& menu = menus[*openMenu];
    const Rect& title = titles[*openMenu];
    const Rect popup{title.x, title.bottom() + 4, menuWidth, menuItemHeight * static_cast<float>(menu.items.size()) + 8};
    ui.beginPopup(popup);
    float y = popup.y + 4;
    std::function<void()> chosen;
    for (const auto& item : menu.items) {
        const Rect row{popup.x + 4, y, popup.w - 8, menuItemHeight};
        if (ui.hovered(row)) ui.draw.fill(row, theme.accentSoft);
        ui.labelLeft(row.inset(10), item.label, theme.text);
        if (!item.shortcut.empty()) ui.labelRight(row.inset(10), item.shortcut, theme.textMuted, TextStyle::Mono);
        if (ui.pressed(row)) chosen = item.action;
        y += menuItemHeight;
    }
    ui.endPopup();
    if (chosen) {
        openMenu.reset();
        chosen();
    } else if (ui.input.mousePressed(MouseButton::Left) && !popup.contains(ui.input.mouse())) {
        openMenu.reset();
    }
}

void Editor::drawStatusBar(Rect bar, float framesPerSecond) {
    const Theme& theme = ui.theme;
    ui.draw.fill(bar, theme.panel);
    ui.draw.line({bar.x, bar.y}, {bar.right(), bar.y}, theme.border);
    float x = bar.x + 16;
    const float y = std::round(bar.y + (bar.h - ui.draw.lineHeight(TextStyle::Mono)) / 2);
    ui.swatch({x + 3, bar.y + bar.h / 2}, theme.highlight, 6);
    x += 14;

    const Scene& scene = state.mode == PlayMode::Editing ? state.scene : state.runtime;
    const std::string items[] = {
        std::format("{:.0f} FPS", framesPerSecond),
        std::format("{} entidades", scene.entities.size()),
        std::format("{} draw calls", ui.draw.drawCalls()),
        std::format("Física {:.2f} ms", physicsMilliseconds),
        "Hot reload ativo (theme.css, sprites)",
    };
    for (const auto& item : items) {
        ui.label({x, y}, item, theme.textMuted, TextStyle::Mono);
        x += ui.draw.textWidth(item, TextStyle::Mono) + 24;
    }
}

}
