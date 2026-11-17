#include "game/Gameplay.hpp"

#include "game/Physics.hpp"
#include "game/Projection.hpp"

#include <algorithm>
#include <cmath>
#include <unordered_set>

namespace pf {

namespace {

constexpr float stompTolerance = 6;
constexpr float stompBounce = 0.6f;
constexpr float shortJumpFactor = 0.4f;

void driveController(Entity& entity, const GameInput& input) {
    auto& controller = *entity.controller;
    auto& body = *entity.body;
    const float direction = static_cast<float>(input.right) - static_cast<float>(input.left);
    body.velocity.x = direction * controller.speed;

    controller.coyoteTimer = body.grounded ? controller.coyoteTime : controller.coyoteTimer - Gameplay::tickSeconds;
    controller.bufferTimer = input.jumpPressed ? controller.jumpBuffer : controller.bufferTimer - Gameplay::tickSeconds;
    if (controller.bufferTimer > 0 && controller.coyoteTimer > 0) {
        body.velocity.y = -controller.jump;
        controller.bufferTimer = 0;
        controller.coyoteTimer = 0;
    }
    if (!input.jumpHeld && body.velocity.y < -controller.jump * shortJumpFactor) {
        body.velocity.y = -controller.jump * shortJumpFactor;
    }

    if (entity.sprite) {
        entity.sprite->animation = !body.grounded ? "jump" : direction != 0 ? "run" : "idle";
        if (direction != 0) entity.sprite->flipX = direction < 0;
    }
}

void driveTopDown(Entity& entity, const GameInput& input, const Projection& projection) {
    const Vec2 screenDirection{static_cast<float>(input.right) - static_cast<float>(input.left),
                               static_cast<float>(input.down) - static_cast<float>(input.up)};
    const Vec2 worldDirection = projection.toWorld(screenDirection);
    const float length = std::hypot(worldDirection.x, worldDirection.y);
    const Vec2 velocity = length == 0 ? Vec2{} : worldDirection * (entity.topDown->speed / length);
    entity.body->velocity = velocity;

    if (entity.sprite) {
        entity.sprite->animation = length == 0 ? "idle" : "run";
        if (screenDirection.x != 0) entity.sprite->flipX = screenDirection.x < 0;
    }
}

void drivePatrol(Entity& entity, const Scene& scene) {
    auto& patrol = *entity.patrol;
    auto& body = *entity.body;
    const Rect collider = entity.worldCollider();
    const float frontX = patrol.direction > 0 ? collider.right() + 1 : collider.x - 1;
    const bool ledgeAhead = scene.camera == CameraMode::Side && body.grounded &&
                            !Physics::solidAt(scene, {frontX, collider.bottom() + 1});
    if (body.hitWall || ledgeAhead) patrol.direction = -patrol.direction;
    body.velocity.x = patrol.direction * patrol.speed;
    if (entity.sprite) entity.sprite->flipX = patrol.direction < 0;
}

void resolveInteractions(Scene& scene, Entity& player) {
    std::unordered_set<int> removed;
    for (auto& other : scene.entities) {
        if (other.id == player.id || !other.worldCollider().overlaps(player.worldCollider())) continue;
        if (other.pickup) {
            scene.state.coins += other.pickup->value;
            removed.insert(other.id);
        } else if (other.patrol) {
            const bool stomp = player.controller && player.body->velocity.y > 0 &&
                               player.worldCollider().bottom() - other.worldCollider().y < stompTolerance;
            if (stomp) {
                removed.insert(other.id);
                player.body->velocity.y = -player.controller->jump * stompBounce;
            } else {
                scene.state.defeated = true;
            }
        }
    }
    std::erase_if(scene.entities, [&](const Entity& entity) { return removed.contains(entity.id); });
}

}

void Gameplay::begin(Scene& scene) {
    scene.state = {};
    for (const auto& entity : scene.entities) {
        if (entity.pickup) scene.state.totalCoins += entity.pickup->value;
    }
    if (const Entity* player = scene.findPlayer()) scene.state.camera = cameraFollowing(scene, *player);
}

void Gameplay::tick(Scene& scene, const GameInput& input) {
    const Projection projection = Projection::of(scene);
    for (auto& entity : scene.entities) {
        if (!entity.body) continue;
        if (entity.controller) driveController(entity, input);
        if (entity.topDown) driveTopDown(entity, input, projection);
        if (entity.patrol) drivePatrol(entity, scene);
    }

    Physics::step(scene, tickSeconds);

    Entity* player = scene.findPlayer();
    if (!player || !player->body) return;
    const int playerId = player->id;
    resolveInteractions(scene, *player);
    player = scene.find(playerId);
    if (player->transform.position.y > scene.tilemap.bounds().bottom()) scene.state.defeated = true;
    scene.state.camera = cameraFollowing(scene, *player);
}

void Gameplay::animate(Scene& scene, float deltaSeconds) {
    for (auto& entity : scene.entities) {
        if (entity.sprite) entity.sprite->animationTime += deltaSeconds;
    }
}

Vec2 Gameplay::cameraFollowing(const Scene& scene, const Entity& target, Vec2 viewSize) {
    const Projection projection = Projection::of(scene);
    const Rect collider = target.worldCollider();
    const Vec2 focus = projection.toView({collider.x + collider.w / 2, collider.y + collider.h / 2});
    const Rect world = projection.viewBounds(scene.tilemap.bounds());
    auto follow = [](float center, float start, float length, float view) {
        if (length <= view) return start + (length - view) / 2;
        return std::clamp(center - view / 2, start, start + length - view);
    };
    return {follow(focus.x, world.x, world.w, viewSize.x), follow(focus.y, world.y, world.h, viewSize.y)};
}

}
