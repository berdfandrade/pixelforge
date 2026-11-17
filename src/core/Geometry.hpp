#pragma once

#include <algorithm>

namespace pf {

struct Vec2 {
    float x = 0;
    float y = 0;

    Vec2 operator+(Vec2 other) const { return {x + other.x, y + other.y}; }
    Vec2 operator-(Vec2 other) const { return {x - other.x, y - other.y}; }
    Vec2 operator*(float factor) const { return {x * factor, y * factor}; }
};

struct Rect {
    float x = 0;
    float y = 0;
    float w = 0;
    float h = 0;

    float right() const { return x + w; }
    float bottom() const { return y + h; }

    bool contains(Vec2 point) const {
        return point.x >= x && point.x < right() && point.y >= y && point.y < bottom();
    }

    bool overlaps(const Rect& other) const {
        return x < other.right() && right() > other.x && y < other.bottom() && bottom() > other.y;
    }

    Rect inset(float amount) const { return {x + amount, y + amount, w - amount * 2, h - amount * 2}; }

    Rect takeTop(float height) {
        Rect taken{x, y, w, height};
        y += height;
        h = std::max(0.0f, h - height);
        return taken;
    }

    Rect takeBottom(float height) {
        h = std::max(0.0f, h - height);
        return {x, y + h, w, height};
    }

    Rect takeLeft(float width) {
        Rect taken{x, y, width, h};
        x += width;
        w = std::max(0.0f, w - width);
        return taken;
    }

    Rect takeRight(float width) {
        w = std::max(0.0f, w - width);
        return {x + w, y, width, h};
    }
};

}
