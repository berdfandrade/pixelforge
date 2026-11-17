#pragma once

#include "game/GameInput.hpp"
#include "scene/Scene.hpp"

namespace pf {

inline constexpr Vec2 gameResolution{320, 180};

class Gameplay {
public:
    static constexpr float tickSeconds = 1.0f / 60.0f;

    static void begin(Scene& scene);
    static void tick(Scene& scene, const GameInput& input);
    static void animate(Scene& scene, float deltaSeconds);
    static Vec2 cameraFollowing(const Scene& scene, const Entity& target, Vec2 viewSize = gameResolution);
};

}
