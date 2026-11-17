#include "scene/SceneSerializer.hpp"

#include "core/Log.hpp"

#include <format>
#include <fstream>
#include <sstream>
#include <string_view>
#include <utility>

namespace pf {

namespace {

std::string restOfLine(std::istringstream& words) {
    std::string rest;
    std::getline(words >> std::ws, rest);
    return rest;
}

std::string hex(Color color) { return std::format("#{:02x}{:02x}{:02x}", color.r, color.g, color.b); }

constexpr std::pair<CameraMode, std::string_view> cameraNames[] = {
    {CameraMode::Side, "side"},
    {CameraMode::TopDown, "topdown"},
    {CameraMode::Isometric, "isometric"},
};

std::string_view cameraName(CameraMode mode) {
    for (const auto& [value, name] : cameraNames) {
        if (value == mode) return name;
    }
    return "side";
}

std::optional<CameraMode> parseCamera(std::string_view name) {
    for (const auto& [value, text] : cameraNames) {
        if (text == name) return value;
    }
    return std::nullopt;
}

Color readColor(std::istringstream& words, Color fallback) {
    std::string text;
    words >> text;
    return Color::fromHex(text).value_or(fallback);
}

class SceneReader {
public:
    explicit SceneReader(Scene& scene) : scene(scene) {}

    void read(const std::string& line) {
        std::istringstream words(line);
        std::string command;
        words >> command;
        if (command == "scene") {
            scene.name = restOfLine(words);
        } else if (command == "camera") {
            std::string mode;
            words >> mode;
            if (auto parsed = parseCamera(mode)) {
                scene.camera = *parsed;
            } else {
                Log::warning("Cena: câmera desconhecida '" + mode + "'");
            }
        } else if (command == "backdrop") {
            scene.backdrop.sky = readColor(words, scene.backdrop.sky);
            scene.backdrop.hills = readColor(words, scene.backdrop.hills);
            scene.backdrop.moon = readColor(words, scene.backdrop.moon);
        } else if (command == "tilemap") {
            int columns = 0;
            int rows = 0;
            words >> columns >> rows;
            scene.tilemap = Tilemap(columns, rows);
            tileRow = 0;
        } else if (command == "row") {
            const std::string cells = restOfLine(words);
            for (int column = 0; column < static_cast<int>(cells.size()); ++column) {
                scene.tilemap.set(column, tileRow, cells[column] == '#');
            }
            ++tileRow;
        } else if (command == "entity") {
            Entity entity;
            words >> entity.id;
            entity.name = restOfLine(words);
            scene.entities.push_back(std::move(entity));
        } else {
            readComponent(command, words);
        }
    }

private:
    void readComponent(const std::string& command, std::istringstream& words) {
        if (scene.entities.empty()) {
            Log::warning("Cena: '" + command + "' antes de qualquer entity");
            return;
        }
        Entity& entity = scene.entities.back();
        if (command == "group") {
            entity.group = restOfLine(words);
        } else if (command == "transform") {
            words >> entity.transform.position.x >> entity.transform.position.y;
        } else if (command == "sprite") {
            Sprite sprite;
            words >> sprite.asset >> sprite.layer;
            entity.sprite = sprite;
        } else if (command == "body") {
            Body body;
            std::string type;
            words >> type >> body.collider.x >> body.collider.y >> body.collider.w >> body.collider.h >> body.gravity;
            body.type = type == "static" ? BodyType::Static : BodyType::Dynamic;
            entity.body = body;
        } else if (command == "controller") {
            PlatformerController controller;
            words >> controller.speed >> controller.jump >> controller.coyoteTime >> controller.jumpBuffer;
            entity.controller = controller;
        } else if (command == "topdown") {
            TopDownController topDown;
            words >> topDown.speed;
            entity.topDown = topDown;
        } else if (command == "patrol") {
            Patrol patrol;
            words >> patrol.speed;
            entity.patrol = patrol;
        } else if (command == "pickup") {
            Pickup pickup;
            words >> pickup.value;
            entity.pickup = pickup;
        } else {
            Log::warning("Cena: comando desconhecido '" + command + "'");
        }
    }

    Scene& scene;
    int tileRow = 0;
};

void writeEntity(std::ostream& out, const Entity& entity) {
    out << std::format("\nentity {} {}\n", entity.id, entity.name);
    if (!entity.group.empty()) out << "group " << entity.group << '\n';
    out << std::format("transform {} {}\n", entity.transform.position.x, entity.transform.position.y);
    if (const auto& sprite = entity.sprite) {
        out << std::format("sprite {} {}\n", sprite->asset, sprite->layer);
    }
    if (const auto& body = entity.body) {
        out << std::format("body {} {} {} {} {} {}\n", body->type == BodyType::Static ? "static" : "dynamic",
                           body->collider.x, body->collider.y, body->collider.w, body->collider.h, body->gravity);
    }
    if (const auto& controller = entity.controller) {
        out << std::format("controller {} {} {} {}\n", controller->speed, controller->jump, controller->coyoteTime,
                           controller->jumpBuffer);
    }
    if (const auto& topDown = entity.topDown) out << std::format("topdown {}\n", topDown->speed);
    if (const auto& patrol = entity.patrol) out << std::format("patrol {}\n", patrol->speed);
    if (const auto& pickup = entity.pickup) out << std::format("pickup {}\n", pickup->value);
}

}

std::optional<Scene> SceneSerializer::load(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file) {
        Log::error("Não foi possível abrir a cena " + path.string());
        return std::nullopt;
    }
    Scene scene;
    scene.entities.clear();
    SceneReader reader(scene);
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (!line.empty()) reader.read(line);
    }
    return scene;
}

bool SceneSerializer::save(const Scene& scene, const std::filesystem::path& path) {
    std::ofstream out(path);
    if (!out) {
        Log::error("Não foi possível salvar a cena em " + path.string());
        return false;
    }
    out << "scene " << scene.name << '\n';
    out << "camera " << cameraName(scene.camera) << '\n';
    out << "backdrop " << hex(scene.backdrop.sky) << ' ' << hex(scene.backdrop.hills) << ' ' << hex(scene.backdrop.moon) << '\n';
    out << std::format("tilemap {} {}\n", scene.tilemap.columns(), scene.tilemap.rows());
    for (int row = 0; row < scene.tilemap.rows(); ++row) {
        std::string cells;
        for (int column = 0; column < scene.tilemap.columns(); ++column) {
            cells += scene.tilemap.solid(column, row) ? '#' : '.';
        }
        out << "row " << cells << '\n';
    }
    for (const auto& entity : scene.entities) writeEntity(out, entity);
    return static_cast<bool>(out);
}

}
