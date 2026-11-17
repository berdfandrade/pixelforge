#include "scene/Scene.hpp"

#include <algorithm>

namespace pf {

Tilemap::Tilemap(int columns, int rows)
    : columnCount(columns), rowCount(rows), cells(static_cast<std::size_t>(columns * rows), false) {}

bool Tilemap::contains(int column, int row) const {
    return column >= 0 && row >= 0 && column < columnCount && row < rowCount;
}

bool Tilemap::solid(int column, int row) const {
    return contains(column, row) && cells[static_cast<std::size_t>(row * columnCount + column)];
}

void Tilemap::set(int column, int row, bool isSolid) {
    if (contains(column, row)) {
        cells[static_cast<std::size_t>(row * columnCount + column)] = isSolid;
    }
}

Rect Tilemap::bounds() const {
    return {0, 0, static_cast<float>(columnCount * tileSize), static_cast<float>(rowCount * tileSize)};
}

Entity& Scene::create(std::string entityName) {
    Entity entity;
    entity.id = nextId();
    entity.name = std::move(entityName);
    entities.push_back(std::move(entity));
    return entities.back();
}

void Scene::remove(int id) {
    std::erase_if(entities, [id](const Entity& entity) { return entity.id == id; });
}

Entity* Scene::find(int id) {
    const auto found = std::find_if(entities.begin(), entities.end(), [id](const Entity& entity) { return entity.id == id; });
    return found == entities.end() ? nullptr : &*found;
}

const Entity* Scene::find(int id) const { return const_cast<Scene*>(this)->find(id); }

Entity* Scene::findPlayer() {
    const auto found = std::find_if(entities.begin(), entities.end(), [](const Entity& entity) { return entity.controller || entity.topDown; });
    return found == entities.end() ? nullptr : &*found;
}

int Scene::nextId() const {
    int highest = 0;
    for (const auto& entity : entities) highest = std::max(highest, entity.id);
    return highest + 1;
}

}
