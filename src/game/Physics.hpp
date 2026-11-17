#pragma once

#include "scene/Scene.hpp"

namespace pf {

class Physics {
public:
    static constexpr float gravity = 900;
    static constexpr float maxFallSpeed = 420;

    static void step(Scene& scene, float deltaSeconds);
    static bool solidAt(const Scene& scene, Vec2 point);
};

}
