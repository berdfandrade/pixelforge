#pragma once

#include "core/Color.hpp"

#include <filesystem>

namespace pf {

struct Theme {
    Color background{0x13, 0x15, 0x1b};
    Color panel{0x1b, 0x1e, 0x26};
    Color card{0x1f, 0x22, 0x2b};
    Color field{0x14, 0x16, 0x1c};
    Color border{0x2a, 0x2e, 0x39};
    Color text{0xe4, 0xe6, 0xeb};
    Color textMuted{0x8a, 0x8f, 0x9c};
    Color accent{0xff, 0x7a, 0x3d};
    Color accentSoft{0x3a, 0x23, 0x18};
    Color highlight{0x4f, 0xd1, 0xc5};
    Color danger{0xff, 0x5c, 0x5c};
    Color warning{0xff, 0xc9, 0x4d};
    Color tagEntity{0x9d, 0x7b, 0xff};
    Color tagTerrain{0x4a, 0x8a, 0x5c};
    Color tagPickup{0xff, 0xc9, 0x4d};

    float headerHeight = 62;
    float toolbarHeight = 52;
    float sidebarWidth = 230;
    float inspectorWidth = 290;
    float bottomHeight = 172;
    float statusHeight = 26;
    float rowHeight = 31;
    float padding = 12;
    float fontSize = 13;
    float monoSize = 12;

    static Theme load(const std::filesystem::path& path);
};

}
