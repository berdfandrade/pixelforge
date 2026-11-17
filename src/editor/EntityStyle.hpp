#pragma once

#include "core/Theme.hpp"
#include "scene/Components.hpp"

namespace pf {

inline Color entityColor(const Entity& entity, const Theme& theme) {
    if (entity.controller || entity.topDown) return theme.accent;
    if (entity.patrol) return theme.tagEntity;
    if (entity.pickup) return theme.tagPickup;
    if (entity.body && entity.body->type == BodyType::Static) return theme.tagTerrain;
    return theme.textMuted;
}

inline int componentCount(const Entity& entity) {
    return 1 + entity.sprite.has_value() + entity.body.has_value() + entity.controller.has_value() + entity.topDown.has_value() +
           entity.patrol.has_value() + entity.pickup.has_value();
}

}
