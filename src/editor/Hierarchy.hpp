#pragma once

#include "editor/EditorState.hpp"
#include "editor/Ui.hpp"

#include <string>

namespace pf {

class Hierarchy {
public:
    void draw(Ui& ui, EditorState& state, Rect area);

private:
    void entityRow(Ui& ui, EditorState& state, const Entity& entity, Rect row, float indent);
    void groupRow(Ui& ui, const std::string& group, Color color, int count, Rect row);

    std::string filter;
    float scroll = 0;
};

}
