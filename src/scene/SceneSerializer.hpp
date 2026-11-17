#pragma once

#include "scene/Scene.hpp"

#include <filesystem>
#include <optional>

namespace pf {

class SceneSerializer {
public:
    static std::optional<Scene> load(const std::filesystem::path& path);
    static bool save(const Scene& scene, const std::filesystem::path& path);
};

}
