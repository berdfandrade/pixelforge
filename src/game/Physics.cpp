#include "game/Physics.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace pf {

namespace {

enum class Axis { Horizontal, Vertical };

std::vector<Rect> solidsOverlapping(const Scene& scene, const Rect& area, int ignoredId) {
    std::vector<Rect> solids;
    const int size = Tilemap::tileSize;
    const int firstColumn = static_cast<int>(std::floor(area.x / size));
    const int lastColumn = static_cast<int>(std::floor((area.right() - 0.001f) / size));
    const int firstRow = static_cast<int>(std::floor(area.y / size));
    const int lastRow = static_cast<int>(std::floor((area.bottom() - 0.001f) / size));
    for (int row = firstRow; row <= lastRow; ++row) {
        for (int column = firstColumn; column <= lastColumn; ++column) {
            const bool outsideHorizontally = column < 0 || column >= scene.tilemap.columns();
            const bool outsideVertically = scene.camera != CameraMode::Side && (row < 0 || row >= scene.tilemap.rows());
            if (outsideHorizontally || outsideVertically || scene.tilemap.solid(column, row)) {
                solids.push_back({static_cast<float>(column * size), static_cast<float>(row * size),
                                  static_cast<float>(size), static_cast<float>(size)});
            }
        }
    }
    for (const auto& other : scene.entities) {
        if (other.id == ignoredId || !other.body || other.body->type != BodyType::Static) continue;
        const Rect rect = other.worldCollider();
        if (rect.overlaps(area)) solids.push_back(rect);
    }
    return solids;
}

bool moveAxis(Entity& entity, float delta, Axis axis, const Scene& scene) {
    if (delta == 0) return false;
    float& coordinate = axis == Axis::Horizontal ? entity.transform.position.x : entity.transform.position.y;
    coordinate += delta;

    const Rect local = entity.body->collider;
    bool collided = false;
    for (const Rect& solid : solidsOverlapping(scene, entity.worldCollider(), entity.id)) {
        if (!solid.overlaps(entity.worldCollider())) continue;
        collided = true;
        if (axis == Axis::Horizontal) {
            coordinate = delta > 0 ? solid.x - local.right() : solid.right() - local.x;
        } else {
            coordinate = delta > 0 ? solid.y - local.bottom() : solid.bottom() - local.y;
        }
    }
    return collided;
}

}

void Physics::step(Scene& scene, float deltaSeconds) {
    const bool hasGravity = scene.camera == CameraMode::Side;
    for (auto& entity : scene.entities) {
        if (!entity.body || entity.body->type != BodyType::Dynamic) continue;
        Body& body = *entity.body;
        if (hasGravity) body.velocity.y = std::min(body.velocity.y + gravity * body.gravity * deltaSeconds, maxFallSpeed);

        body.hitWall = moveAxis(entity, body.velocity.x * deltaSeconds, Axis::Horizontal, scene);
        if (body.hitWall) body.velocity.x = 0;

        const float verticalDelta = body.velocity.y * deltaSeconds;
        const bool hitVertical = moveAxis(entity, verticalDelta, Axis::Vertical, scene);
        body.grounded = hitVertical && verticalDelta > 0;
        if (hitVertical) body.velocity.y = 0;
    }
}

bool Physics::solidAt(const Scene& scene, Vec2 point) {
    return !solidsOverlapping(scene, {point.x, point.y, 0.01f, 0.01f}, -1).empty();
}

}
