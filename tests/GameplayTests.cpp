#include "game/Gameplay.hpp"
#include "game/Projection.hpp"
#include "scene/SceneSerializer.hpp"

#include <cmath>
#include <cstdio>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace {

using namespace pf;

int failures = 0;

void expect(bool condition, const std::string& description) {
    if (!condition) {
        ++failures;
        std::printf("  FALHOU: %s\n", description.c_str());
    }
}

Body bodyWith(Rect collider) {
    Body body;
    body.collider = collider;
    return body;
}

Scene flatGround() {
    Scene scene;
    scene.tilemap = Tilemap(20, 12);
    for (int column = 0; column < 20; ++column) scene.tilemap.set(column, 10, true);
    Entity& player = scene.create("Player");
    player.transform.position = {32, 100};
    player.body = bodyWith({1, 1, 10, 15});
    player.controller = PlatformerController{};
    return scene;
}

Entity& player(Scene& scene) { return *scene.findPlayer(); }

void run(Scene& scene, int ticks, GameInput input = {}) {
    for (int tick = 0; tick < ticks; ++tick) {
        Gameplay::tick(scene, input);
        input.jumpPressed = false;
    }
}

void playerLandsOnGround() {
    Scene scene = flatGround();
    run(scene, 90);
    expect(player(scene).body->grounded, "player fica no chão");
    expect(player(scene).worldCollider().bottom() == 160, "colisor encosta no topo do tile");
}

void playerWalksRight() {
    Scene scene = flatGround();
    run(scene, 90);
    const float start = player(scene).transform.position.x;
    run(scene, 30, {.right = true});
    const float travelled = player(scene).transform.position.x - start;
    expect(std::abs(travelled - 45) < 1, "anda velocidade * tempo (" + std::to_string(travelled) + ")");
}

void playerJumpsAndShortHopIsLower() {
    auto apex = [](bool hold) {
        Scene scene = flatGround();
        run(scene, 90);
        const float ground = player(scene).transform.position.y;
        float highest = ground;
        GameInput input{.jumpPressed = true, .jumpHeld = hold};
        for (int tick = 0; tick < 60; ++tick) {
            Gameplay::tick(scene, input);
            input.jumpPressed = false;
            highest = std::min(highest, player(scene).transform.position.y);
        }
        return ground - highest;
    };
    const float full = apex(true);
    const float shortHop = apex(false);
    expect(full > 20, "pulo segurado sobe (" + std::to_string(full) + ")");
    expect(shortHop < full, "soltar o botão encurta o pulo");
}

void coyoteTimeAllowsLateJump() {
    Scene scene = flatGround();
    for (int column = 4; column < 20; ++column) scene.tilemap.set(column, 10, false);
    run(scene, 90);
    for (int tick = 0; tick < 200 && player(scene).body->grounded; ++tick) Gameplay::tick(scene, {.right = true});
    expect(!player(scene).body->grounded, "player saiu da borda");
    run(scene, 3, {.right = true});
    Gameplay::tick(scene, {.right = true, .jumpPressed = true, .jumpHeld = true});
    expect(player(scene).body->velocity.y < 0, "pula logo após sair da borda (coyote time)");
}

void jumpBufferJumpsOnLanding() {
    Scene scene = flatGround();
    run(scene, 90);
    run(scene, 1, {.jumpPressed = true, .jumpHeld = true});
    while (player(scene).body->velocity.y <= 0) Gameplay::tick(scene, {.jumpHeld = true});
    while (player(scene).worldCollider().bottom() < 160 - 6) Gameplay::tick(scene, {});
    Gameplay::tick(scene, {.jumpPressed = true, .jumpHeld = true});
    for (int tick = 0; tick < 10 && player(scene).body->velocity.y >= 0; ++tick) Gameplay::tick(scene, {.jumpHeld = true});
    expect(player(scene).body->velocity.y < 0, "pulo pressionado antes de pousar é executado (jump buffer)");
}

void coinIsCollected() {
    Scene scene = flatGround();
    Entity& coin = scene.create("Moeda");
    coin.transform.position = {60, 148};
    coin.pickup = Pickup{1};
    Gameplay::begin(scene);
    run(scene, 90, {.right = true});
    expect(scene.state.coins == 1 && scene.state.totalCoins == 1, "moeda coletada");
    expect(scene.entities.size() == 1, "moeda removida da cena");
}

void enemyStompAndContact() {
    Scene stomp = flatGround();
    Entity& slime = stomp.create("Slime");
    slime.transform.position = {31, 150};
    slime.body = bodyWith({1, 2, 14, 8});
    slime.patrol = Patrol{0};
    player(stomp).transform.position = {32, 60};
    run(stomp, 60);
    expect(!stomp.state.defeated && stomp.entities.size() == 1, "cair em cima elimina o inimigo");

    Scene contact = flatGround();
    Entity& walker = contact.create("Slime");
    walker.transform.position = {80, 150};
    walker.body = bodyWith({1, 2, 14, 8});
    walker.patrol = Patrol{0};
    run(contact, 90, {.right = true});
    expect(contact.state.defeated, "encostar de lado derrota o player");
}

Scene openField(CameraMode mode) {
    Scene scene;
    scene.camera = mode;
    scene.tilemap = Tilemap(20, 20);
    Entity& hero = scene.create("Player");
    hero.transform.position = {150, 150};
    hero.body = bodyWith({1, 10, 10, 6});
    hero.topDown = TopDownController{60};
    return scene;
}

void topDownMovesInEightDirections() {
    Scene scene = openField(CameraMode::TopDown);
    run(scene, 60, {.right = true, .down = true});
    const Vec2 moved = player(scene).transform.position - Vec2{150, 150};
    expect(moved.x > 0 && std::abs(moved.x - moved.y) < 0.01f, "diagonal move igual nos dois eixos");
    expect(std::abs(std::hypot(moved.x, moved.y) - 60) < 1, "diagonal normalizada (" + std::to_string(std::hypot(moved.x, moved.y)) + ")");

    Scene idle = openField(CameraMode::TopDown);
    run(idle, 60);
    expect(player(idle).transform.position.y == 150, "sem gravidade no RPG");
}

void topDownStopsAtWalls() {
    Scene scene = openField(CameraMode::TopDown);
    for (int row = 0; row < 20; ++row) scene.tilemap.set(12, row, true);
    run(scene, 120, {.right = true});
    expect(player(scene).worldCollider().right() == 192, "parede bloqueia o movimento");

    Scene edge = openField(CameraMode::TopDown);
    run(edge, 400, {.up = true});
    expect(player(edge).worldCollider().y == 0, "borda do mapa bloqueia no RPG");
}

void isometricInputFollowsTheScreen() {
    Scene scene = openField(CameraMode::Isometric);
    run(scene, 30, {.up = true});
    const Vec2 moved = player(scene).transform.position - Vec2{150, 150};
    expect(moved.x < 0 && std::abs(moved.x - moved.y) < 0.01f, "cima na tela anda na diagonal (-x, -y) do mundo");

    const Projection projection(CameraMode::Isometric);
    const Vec2 point{37, 81};
    const Vec2 back = projection.toWorld(projection.toView(point));
    expect(std::abs(back.x - point.x) < 0.001f && std::abs(back.y - point.y) < 0.001f, "projeção isométrica é inversível");
    expect(projection.toView({16, 0}).x == 16 && projection.toView({16, 0}).y == 8, "tile isométrico 2:1");
}

void sampleScenesStartClearOfWalls() {
    for (const auto& entry : std::filesystem::directory_iterator(std::filesystem::path(PF_ASSETS_DIR) / "scenes")) {
        const auto scene = SceneSerializer::load(entry.path());
        expect(scene.has_value(), entry.path().filename().string() + " carrega");
        if (!scene) continue;
        for (const auto& entity : scene->entities) {
            const Rect area = entity.worldCollider();
            bool blocked = false;
            for (float y = area.y; y < area.bottom(); y += 1) {
                for (float x = area.x; x < area.right(); x += 1) blocked = blocked || scene->tilemap.solid(static_cast<int>(x) / 16, static_cast<int>(y) / 16);
            }
            expect(!blocked, entry.path().filename().string() + ": " + entity.name + " começa fora das paredes");
        }
    }
}

void sceneRoundTrips() {
    const auto original = SceneSerializer::load(std::filesystem::path(PF_ASSETS_DIR) / "scenes" / "level1.scene");
    expect(original.has_value(), "level1.scene carrega");
    if (!original) return;
    const auto path = std::filesystem::temp_directory_path() / "pixelforge_roundtrip.scene";
    expect(SceneSerializer::save(*original, path), "cena salva");
    const auto reloaded = SceneSerializer::load(path);
    std::filesystem::remove(path);
    expect(reloaded && reloaded->entities.size() == original->entities.size(), "mesmo número de entidades");
    expect(reloaded && reloaded->tilemap.solid(0, 10) && !reloaded->tilemap.solid(24, 10), "tiles preservados");
    expect(reloaded && reloaded->entities.front().controller->jump == 220, "componentes preservados");

    const auto rpg = SceneSerializer::load(std::filesystem::path(PF_ASSETS_DIR) / "scenes" / "rpg.scene");
    expect(rpg && rpg->camera == CameraMode::TopDown, "rpg.scene usa câmera de cima");
    if (!rpg) return;
    expect(SceneSerializer::save(*rpg, path), "rpg salva");
    const auto rpgReloaded = SceneSerializer::load(path);
    std::filesystem::remove(path);
    expect(rpgReloaded && rpgReloaded->camera == CameraMode::TopDown && rpgReloaded->entities.front().topDown->speed == 70,
           "modo de câmera e TopDownController preservados");
}

}

int main() {
    const std::vector<std::pair<const char*, std::function<void()>>> tests{
        {"player pousa no chão", playerLandsOnGround},
        {"player anda", playerWalksRight},
        {"pulo variável", playerJumpsAndShortHopIsLower},
        {"coyote time", coyoteTimeAllowsLateJump},
        {"jump buffer", jumpBufferJumpsOnLanding},
        {"coleta de moeda", coinIsCollected},
        {"inimigos", enemyStompAndContact},
        {"RPG: 8 direções", topDownMovesInEightDirections},
        {"RPG: paredes", topDownStopsAtWalls},
        {"isométrico", isometricInputFollowsTheScreen},
        {"cenas de exemplo", sampleScenesStartClearOfWalls},
        {"serialização", sceneRoundTrips},
    };
    for (const auto& [name, test] : tests) {
        const int before = failures;
        test();
        std::printf("%s %s\n", failures == before ? "ok  " : "FAIL", name);
    }
    return failures == 0 ? 0 : 1;
}
