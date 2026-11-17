#include "render/SpriteLibrary.hpp"

#include "core/Color.hpp"
#include "core/Log.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <optional>
#include <sstream>
#include <unordered_map>

namespace pf {

namespace {

struct FrameRows {
    std::vector<std::string> rows;
};

struct AnimationSource {
    std::string name;
    float framesPerSecond = 1;
    std::vector<FrameRows> frames;
};

struct SpriteSource {
    int width = 0;
    int height = 0;
    std::unordered_map<char, Color> palette;
    std::vector<AnimationSource> animations;
};

class SpriteParseError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

SpriteSource parse(std::istream& stream) {
    SpriteSource source;
    std::string line;
    int lineNumber = 0;
    auto fail = [&](const std::string& reason) {
        throw SpriteParseError("linha " + std::to_string(lineNumber) + ": " + reason);
    };
    while (std::getline(stream, line)) {
        ++lineNumber;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;

        std::istringstream words(line);
        std::string command;
        words >> command;
        if (command == "size") {
            words >> source.width >> source.height;
        } else if (command == "color") {
            std::string key;
            std::string hex;
            words >> key >> hex;
            const auto color = Color::fromHex(hex);
            if (key.size() != 1 || !color) fail("cor inválida");
            source.palette[key[0]] = *color;
        } else if (command == "anim") {
            AnimationSource animation;
            words >> animation.name >> animation.framesPerSecond;
            source.animations.push_back(std::move(animation));
        } else if (command == "frame") {
            if (source.animations.empty()) fail("frame fora de anim");
            source.animations.back().frames.emplace_back();
        } else {
            if (source.animations.empty() || source.animations.back().frames.empty()) fail("pixels fora de frame");
            if (static_cast<int>(line.size()) != source.width) fail("largura da linha diferente de size");
            source.animations.back().frames.back().rows.push_back(line);
        }
    }
    if (source.width <= 0 || source.height <= 0) throw SpriteParseError("size ausente");
    for (const auto& animation : source.animations) {
        for (const auto& frame : animation.frames) {
            if (static_cast<int>(frame.rows.size()) != source.height) {
                throw SpriteParseError("frame de '" + animation.name + "' com altura diferente de size");
            }
        }
    }
    return source;
}

TextureHandle createFrameTexture(SDL_Renderer* renderer, const SpriteSource& source, const FrameRows& frame) {
    SDL_Surface* surface = SDL_CreateSurface(source.width, source.height, SDL_PIXELFORMAT_RGBA32);
    if (!surface) return nullptr;
    auto* pixels = static_cast<std::uint8_t*>(surface->pixels);
    for (int y = 0; y < source.height; ++y) {
        for (int x = 0; x < source.width; ++x) {
            const auto found = source.palette.find(frame.rows[y][x]);
            const Color color = found == source.palette.end() ? Color{0, 0, 0, 0} : found->second;
            std::uint8_t* pixel = pixels + y * surface->pitch + x * 4;
            pixel[0] = color.r;
            pixel[1] = color.g;
            pixel[2] = color.b;
            pixel[3] = color.a;
        }
    }
    TextureHandle texture(SDL_CreateTextureFromSurface(renderer, surface));
    SDL_DestroySurface(surface);
    if (texture) SDL_SetTextureScaleMode(texture.get(), SDL_SCALEMODE_NEAREST);
    return texture;
}

std::optional<SpriteAsset> loadSprite(SDL_Renderer* renderer, const std::filesystem::path& path) {
    std::ifstream file(path);
    try {
        const SpriteSource source = parse(file);
        SpriteAsset asset{path.stem().string(), source.width, source.height, {}};
        for (const auto& animationSource : source.animations) {
            Animation animation;
            animation.framesPerSecond = animationSource.framesPerSecond;
            for (const auto& frame : animationSource.frames) {
                animation.frames.push_back(createFrameTexture(renderer, source, frame));
            }
            asset.animations.emplace(animationSource.name, std::move(animation));
        }
        return asset;
    } catch (const SpriteParseError& error) {
        Log::error(path.filename().string() + ": " + error.what());
        return std::nullopt;
    }
}

}

SDL_Texture* SpriteAsset::frame(const std::string& animation, float time) const {
    auto found = animations.find(animation);
    if (found == animations.end()) found = animations.begin();
    if (found == animations.end() || found->second.frames.empty()) return nullptr;
    const auto& frames = found->second.frames;
    const auto index = static_cast<std::size_t>(std::floor(time * found->second.framesPerSecond)) % frames.size();
    return frames[index].get();
}

SDL_Texture* SpriteAsset::preview() const { return frame("idle", 0); }

SpriteLibrary::SpriteLibrary(SDL_Renderer* renderer, std::filesystem::path directory)
    : renderer(renderer), directory(std::move(directory)) {
    reload();
}

void SpriteLibrary::reload() {
    std::vector<std::filesystem::path> paths;
    std::error_code error;
    for (const auto& entry : std::filesystem::directory_iterator(directory, error)) {
        if (entry.path().extension() == ".sprite") paths.push_back(entry.path());
    }
    std::sort(paths.begin(), paths.end());

    std::vector<SpriteAsset> loaded;
    for (const auto& path : paths) {
        if (auto asset = loadSprite(renderer, path)) loaded.push_back(std::move(*asset));
    }
    sprites = std::move(loaded);
}

const SpriteAsset* SpriteLibrary::find(const std::string& name) const {
    const auto found = std::find_if(sprites.begin(), sprites.end(), [&](const SpriteAsset& asset) { return asset.name == name; });
    return found == sprites.end() ? nullptr : &*found;
}

}
