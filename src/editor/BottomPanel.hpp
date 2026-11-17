#pragma once

#include "editor/EditorState.hpp"
#include "editor/Ui.hpp"

namespace pf {

class BottomPanel {
public:
    void draw(Ui& ui, EditorState& state, Rect area);

private:
    void drawAssets(Ui& ui, EditorState& state, Rect area);
    void drawConsole(Ui& ui, Rect area);

    float assetScroll = 0;
};

}
