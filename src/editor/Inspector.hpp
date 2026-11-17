#pragma once

#include "editor/EditorState.hpp"
#include "editor/Ui.hpp"

namespace pf {

class Inspector {
public:
    void draw(Ui& ui, EditorState& state, Rect area);

private:
    float drawEntityHeader(Ui& ui, EditorState& state, Entity& entity, Rect area);
    void drawSceneSettings(Ui& ui, EditorState& state, Rect area);
    void drawAddMenu(Ui& ui, EditorState& state, Entity& entity, const Rect& anchor);

    float scroll = 0;
    bool addMenuOpen = false;
};

}
