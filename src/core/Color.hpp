#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

namespace pf {

struct Color {
    std::uint8_t r = 0;
    std::uint8_t g = 0;
    std::uint8_t b = 0;
    std::uint8_t a = 255;

    Color withAlpha(std::uint8_t alpha) const { return {r, g, b, alpha}; }

    static std::optional<Color> fromHex(std::string_view text) {
        if (text.starts_with('#')) {
            text.remove_prefix(1);
        }
        if (text.size() != 6 && text.size() != 8) {
            return std::nullopt;
        }
        std::uint32_t value = 0;
        for (char digit : text) {
            int nibble = hexDigit(digit);
            if (nibble < 0) {
                return std::nullopt;
            }
            value = value << 4 | static_cast<std::uint32_t>(nibble);
        }
        if (text.size() == 6) {
            value = value << 8 | 0xff;
        }
        return Color{static_cast<std::uint8_t>(value >> 24), static_cast<std::uint8_t>(value >> 16),
                     static_cast<std::uint8_t>(value >> 8), static_cast<std::uint8_t>(value)};
    }

private:
    static int hexDigit(char digit) {
        if (digit >= '0' && digit <= '9') return digit - '0';
        if (digit >= 'a' && digit <= 'f') return digit - 'a' + 10;
        if (digit >= 'A' && digit <= 'F') return digit - 'A' + 10;
        return -1;
    }
};

}
