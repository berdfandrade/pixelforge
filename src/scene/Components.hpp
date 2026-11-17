#pragma once

#include "core/Geometry.hpp"

#include <optional>
#include <string>

namespace pf {

struct Transform {
    Vec2 position;
};

struct Sprite {
    std::string asset;
    int layer = 0;
    std::string animation = "idle";
    float animationTime = 0;
    bool flipX = false;
};

enum class BodyType { Dynamic, Static };

struct Body {
    BodyType type = BodyType::Dynamic;
    Rect collider{0, 0, 16, 16};
    float gravity = 1;
    Vec2 velocity;
    bool grounded = false;
    bool hitWall = false;
};

struct PlatformerController {
    float speed = 90;
    float jump = 220;
    float coyoteTime = 0.1f;
    float jumpBuffer = 0.12f;
    float coyoteTimer = 0;
    float bufferTimer = 0;
};

struct TopDownController {
    float speed = 70;
};

struct Patrol {
    float speed = 25;
    float direction = 1;
};

struct Pickup {
    int value = 1;
};

struct Entity {
    int id = 0;
    std::string name;
    std::string group;
    Transform transform;
    std::optional<Sprite> sprite;
    std::optional<Body> body;
    std::optional<PlatformerController> controller;
    std::optional<TopDownController> topDown;
    std::optional<Patrol> patrol;
    std::optional<Pickup> pickup;

    Rect worldCollider() const {
        const Rect local = body ? body->collider : Rect{0, 0, 8, 8};
        return {transform.position.x + local.x, transform.position.y + local.y, local.w, local.h};
    }
};

}
