#include "App.hpp"

#include <SDL3/SDL_main.h>

#include <exception>
#include <string_view>

int main(int argc, char* argv[]) {
    pf::AppOptions options{PF_ASSETS_DIR, std::filesystem::path(PF_ASSETS_DIR) / "scenes" / "level1.scene", false, std::nullopt};
    for (int index = 1; index < argc; ++index) {
        const std::string_view argument = argv[index];
        if (argument == "--play") {
            options.playOnly = true;
        } else if (argument == "--screenshot" && index + 1 < argc) {
            options.screenshot = argv[++index];
        } else {
            options.scene = argument;
        }
    }
    try {
        pf::App app(std::move(options));
        return app.run();
    } catch (const std::exception& error) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "PixelForge", error.what(), nullptr);
        return 1;
    }
}
