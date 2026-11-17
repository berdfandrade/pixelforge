#pragma once

#include "core/Color.hpp"
#include "scene/Components.hpp"

#include <string>
#include <vector>

namespace pf {

class Tilemap {
public:
    static constexpr int tileSize = 16;

    Tilemap() = default;
    Tilemap(int columns, int rows);

    int columns() const { return columnCount; }
    int rows() const { return rowCount; }
    bool contains(int column, int row) const;
    bool solid(int column, int row) const;
    void set(int column, int row, bool isSolid);
    Rect bounds() const;

private:
    int columnCount = 0;
    int rowCount = 0;
    std::vector<bool> cells;
};

enum class CameraMode { Side, TopDown, Isometric };

struct Backdrop {
    Color sky{0x26, 0x2d, 0x4a};
    Color hills{0x2e, 0x37, 0x5c};
    Color moon{0xe8, 0xe6, 0xdf};
};

struct GameState {
    int coins = 0;
    int totalCoins = 0;
    bool defeated = false;
    Vec2 camera;
};

class Scene {
public:
    std::string name = "untitled";
    CameraMode camera = CameraMode::Side;
    Backdrop backdrop;
    Tilemap tilemap{40, 12};
    std::vector<Entity> entities;
    GameState state;

    Entity& create(std::string entityName);
    void remove(int id);
    Entity* find(int id);
    const Entity* find(int id) const;
    Entity* findPlayer();
    int nextId() const;
};

}
