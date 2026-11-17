#include "core/Theme.hpp"

#include "core/Log.hpp"

#include <charconv>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <variant>

namespace pf {

namespace {

using Property = std::variant<Color Theme::*, float Theme::*>;

struct Binding {
    std::string_view name;
    Property property;
};

constexpr Binding bindings[] = {
    {"background", &Theme::background},
    {"panel", &Theme::panel},
    {"card", &Theme::card},
    {"field", &Theme::field},
    {"border", &Theme::border},
    {"text", &Theme::text},
    {"text-muted", &Theme::textMuted},
    {"accent", &Theme::accent},
    {"accent-soft", &Theme::accentSoft},
    {"highlight", &Theme::highlight},
    {"danger", &Theme::danger},
    {"warning", &Theme::warning},
    {"tag-entity", &Theme::tagEntity},
    {"tag-terrain", &Theme::tagTerrain},
    {"tag-pickup", &Theme::tagPickup},
    {"header-height", &Theme::headerHeight},
    {"toolbar-height", &Theme::toolbarHeight},
    {"sidebar-width", &Theme::sidebarWidth},
    {"inspector-width", &Theme::inspectorWidth},
    {"bottom-height", &Theme::bottomHeight},
    {"status-height", &Theme::statusHeight},
    {"row-height", &Theme::rowHeight},
    {"padding", &Theme::padding},
    {"font-size", &Theme::fontSize},
    {"mono-size", &Theme::monoSize},
};

std::string_view trim(std::string_view text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string_view::npos) {
        return {};
    }
    const auto last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

std::string stripComments(std::string_view source) {
    std::string result;
    std::size_t position = 0;
    while (position < source.size()) {
        const auto start = source.find("/*", position);
        if (start == std::string_view::npos) {
            result.append(source.substr(position));
            break;
        }
        result.append(source.substr(position, start - position));
        const auto end = source.find("*/", start + 2);
        position = end == std::string_view::npos ? source.size() : end + 2;
    }
    return result;
}

std::optional<float> parseLength(std::string_view value) {
    if (value.ends_with("px")) {
        value.remove_suffix(2);
    }
    float number = 0;
    const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), number);
    if (error != std::errc{} || end != value.data() + value.size()) {
        return std::nullopt;
    }
    return number;
}

bool apply(Theme& theme, const Binding& binding, std::string_view value) {
    if (const auto* color = std::get_if<Color Theme::*>(&binding.property)) {
        const auto parsed = Color::fromHex(value);
        if (parsed) theme.*(*color) = *parsed;
        return parsed.has_value();
    }
    const auto parsed = parseLength(value);
    if (parsed) theme.*std::get<float Theme::*>(binding.property) = *parsed;
    return parsed.has_value();
}

void applyDeclaration(Theme& theme, std::string_view declaration) {
    const auto colon = declaration.find(':');
    if (colon == std::string_view::npos) {
        return;
    }
    auto name = trim(declaration.substr(0, colon));
    const auto value = trim(declaration.substr(colon + 1));
    if (!name.starts_with("--")) {
        return;
    }
    name.remove_prefix(2);
    for (const auto& binding : bindings) {
        if (binding.name != name) {
            continue;
        }
        if (!apply(theme, binding, value)) {
            Log::warning("theme.css: valor inválido para --" + std::string(name));
        }
        return;
    }
    Log::warning("theme.css: variável desconhecida --" + std::string(name));
}

}

Theme Theme::load(const std::filesystem::path& path) {
    Theme theme;
    std::ifstream file(path);
    if (!file) {
        Log::warning("Tema não encontrado: " + path.string());
        return theme;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    const auto source = stripComments(buffer.str());

    const auto open = source.find('{');
    const auto close = source.rfind('}');
    if (open == std::string::npos || close == std::string::npos || close < open) {
        Log::warning("theme.css: bloco :root { ... } não encontrado");
        return theme;
    }
    std::string_view body(source.data() + open + 1, close - open - 1);
    while (!body.empty()) {
        const auto semicolon = body.find(';');
        applyDeclaration(theme, body.substr(0, semicolon));
        if (semicolon == std::string_view::npos) {
            break;
        }
        body.remove_prefix(semicolon + 1);
    }
    return theme;
}

}
