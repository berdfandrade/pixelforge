#pragma once

#include "scene/Scene.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace pf {

class Projection {
public:
    explicit Projection(CameraMode mode) : mode(mode) {}
    static Projection of(const Scene& scene) { return Projection(scene.camera); }

    bool isometric() const { return mode == CameraMode::Isometric; }

    Vec2 toView(Vec2 world) const {
        if (!isometric()) return world;
        return {world.x - world.y, (world.x + world.y) / 2};
    }

    Vec2 toWorld(Vec2 view) const {
        if (!isometric()) return view;
        return {view.y + view.x / 2, view.y - view.x / 2};
    }

    Vec2 viewDirection(Vec2 worldDirection) const {
        const Vec2 direction = toView(worldDirection);
        const float length = std::hypot(direction.x, direction.y);
        return length == 0 ? direction : direction * (1 / length);
    }

    std::array<Vec2, 4> outline(const Rect& world) const {
        return {toView({world.x, world.y}), toView({world.right(), world.y}), toView({world.right(), world.bottom()}),
                toView({world.x, world.bottom()})};
    }

    Rect viewBounds(const Rect& world) const {
        const auto corners = outline(world);
        Vec2 low = corners[0];
        Vec2 high = corners[0];
        for (const Vec2 corner : corners) {
            low = {std::min(low.x, corner.x), std::min(low.y, corner.y)};
            high = {std::max(high.x, corner.x), std::max(high.y, corner.y)};
        }
        return {low.x, low.y, high.x - low.x, high.y - low.y};
    }

private:
    CameraMode mode;
};

}
