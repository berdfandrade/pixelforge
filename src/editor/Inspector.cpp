#include "editor/Inspector.hpp"

#include "editor/EntityStyle.hpp"

#include <algorithm>
#include <charconv>
#include <format>
#include <functional>
#include <string>
#include <vector>

namespace pf {

namespace {

constexpr float headerHeight = 50;
constexpr float cardTitleHeight = 36;
constexpr float fieldRowHeight = 58;
constexpr float fieldLabelGap = 20;
constexpr float fieldHeight = 30;
constexpr float cardGap = 12;
constexpr float menuItemHeight = 28;
constexpr float scrollSpeed = 32;

struct Field {
    std::string label;
    std::string value;
    std::function<void(const std::string&)> apply;
};

struct Card {
    std::string title;
    std::string tag;
    Color tagColor;
    std::vector<Field> fields;
    std::function<void(Entity&)> remove;
};

template <typename Number>
std::optional<Number> parseNumber(std::string_view text) {
    const auto first = text.find_first_not_of(' ');
    if (first == std::string_view::npos) return std::nullopt;
    text.remove_prefix(first);
    Number value{};
    const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
    if (result.ec != std::errc{}) return std::nullopt;
    return value;
}

template <typename Number>
Field numberField(std::string label, Number& target) {
    return {std::move(label), std::format("{}", target), [&target](const std::string& text) {
                if (auto value = parseNumber<Number>(text)) target = *value;
            }};
}

Field textField(std::string label, std::string& target) {
    return {std::move(label), target, [&target](const std::string& text) { target = text; }};
}

Field bodyTypeField(Body& body) {
    return {"tipo", body.type == BodyType::Static ? "static" : "dynamic", [&body](const std::string& text) {
                if (text == "static") body.type = BodyType::Static;
                if (text == "dynamic") body.type = BodyType::Dynamic;
            }};
}

std::vector<Card> buildCards(Entity& entity, const Theme& theme) {
    std::vector<Card> cards;
    auto& transform = entity.transform;
    cards.push_back({"Transform", "core", theme.textMuted,
                     {numberField("x", transform.position.x), numberField("y", transform.position.y)}, nullptr});
    if (entity.sprite) {
        auto& sprite = *entity.sprite;
        cards.push_back({"Sprite", "render", theme.highlight,
                         {textField("sprite", sprite.asset), numberField("camada", sprite.layer)},
                         [](Entity& target) { target.sprite.reset(); }});
    }
    if (entity.body) {
        auto& body = *entity.body;
        cards.push_back({"Body", "física", theme.highlight,
                         {bodyTypeField(body), numberField("gravidade", body.gravity), numberField("largura", body.collider.w),
                          numberField("altura", body.collider.h), numberField("offset x", body.collider.x),
                          numberField("offset y", body.collider.y)},
                         [](Entity& target) { target.body.reset(); }});
    }
    if (entity.controller) {
        auto& controller = *entity.controller;
        cards.push_back({"PlatformerController", "script", theme.accent,
                         {numberField("velocidade", controller.speed), numberField("pulo", controller.jump),
                          numberField("coyote time (s)", controller.coyoteTime),
                          numberField("jump buffer (s)", controller.jumpBuffer)},
                         [](Entity& target) { target.controller.reset(); }});
    }
    if (entity.topDown) {
        cards.push_back({"TopDownController", "script", theme.accent, {numberField("velocidade", entity.topDown->speed)},
                         [](Entity& target) { target.topDown.reset(); }});
    }
    if (entity.patrol) {
        cards.push_back({"Patrol", "ia", theme.tagEntity, {numberField("velocidade", entity.patrol->speed)},
                         [](Entity& target) { target.patrol.reset(); }});
    }
    if (entity.pickup) {
        cards.push_back({"Pickup", "item", theme.tagPickup, {numberField("valor", entity.pickup->value)},
                         [](Entity& target) { target.pickup.reset(); }});
    }
    return cards;
}

float cardHeight(const Card& card) {
    const auto rows = (card.fields.size() + 1) / 2;
    return cardTitleHeight + static_cast<float>(rows) * fieldRowHeight + 4;
}

struct AddOption {
    std::string_view name;
    std::function<bool(const Entity&)> present;
    std::function<void(Entity&, const EditorState&)> add;
};

std::vector<AddOption> addOptions() {
    return {
        {"Sprite", [](const Entity& e) { return e.sprite.has_value(); },
         [](Entity& e, const EditorState& state) {
             e.sprite = Sprite{state.sprites.all().empty() ? std::string{} : state.sprites.all().front().name};
         }},
        {"Body", [](const Entity& e) { return e.body.has_value(); },
         [](Entity& e, const EditorState& state) {
             const Vec2 size = state.gameRenderer.spriteSize(e);
             Body body;
             body.collider = e.sprite ? Rect{0, 0, size.x, size.y} : body.collider;
             e.body = body;
         }},
        {"PlatformerController", [](const Entity& e) { return e.controller.has_value(); },
         [](Entity& e, const EditorState&) { e.controller = PlatformerController{}; }},
        {"TopDownController", [](const Entity& e) { return e.topDown.has_value(); },
         [](Entity& e, const EditorState&) { e.topDown = TopDownController{}; }},
        {"Patrol", [](const Entity& e) { return e.patrol.has_value(); },
         [](Entity& e, const EditorState&) { e.patrol = Patrol{}; }},
        {"Pickup", [](const Entity& e) { return e.pickup.has_value(); },
         [](Entity& e, const EditorState&) { e.pickup = Pickup{}; }},
    };
}

}

void Inspector::draw(Ui& ui, EditorState& state, Rect area) {
    const Theme& theme = ui.theme;
    ui.panel(area, theme.panel);
    Rect content = area;
    const Rect header = content.takeTop(headerHeight);
    ui.sectionTitle({header.x + 14, header.y + 16, header.w, 28}, "Inspetor");

    Entity* entity = state.selectedEntity();
    if (!entity) {
        addMenuOpen = false;
        drawSceneSettings(ui, state, content.inset(theme.padding));
        return;
    }

    content = content.inset(theme.padding);
    const float entityHeaderHeight = drawEntityHeader(ui, state, *entity, content);
    content.takeTop(entityHeaderHeight);

    if (ui.hovered(content)) scroll = std::max(0.0f, scroll - ui.input.wheel() * scrollSpeed);
    ui.draw.pushClip(content);
    float y = content.y - scroll;

    std::function<void(Entity&)> pendingRemoval;
    const auto cards = buildCards(*entity, theme);
    for (std::size_t cardIndex = 0; cardIndex < cards.size(); ++cardIndex) {
        const Card& card = cards[cardIndex];
        const Rect box{content.x, y, content.w, cardHeight(card)};
        ui.panel(box, theme.card);
        ui.labelLeft({box.x + 10, box.y, box.w, cardTitleHeight}, card.title, theme.text, TextStyle::Strong);
        Rect titleRow{box.x, box.y, box.w - 10, cardTitleHeight};
        if (card.remove) {
            const Rect removeArea = titleRow.takeRight(22);
            if (ui.button({removeArea.x, removeArea.y + 7, 22, 22}, "x", ButtonStyle::Ghost)) pendingRemoval = card.remove;
            titleRow.takeRight(4);
        }
        ui.labelRight(titleRow, card.tag, card.tagColor, TextStyle::Mono);

        const float columnWidth = (box.w - 30) / 2;
        for (std::size_t index = 0; index < card.fields.size(); ++index) {
            const Field& field = card.fields[index];
            const float x = box.x + 10 + static_cast<float>(index % 2) * (columnWidth + 10);
            const float fieldY = box.y + cardTitleHeight + static_cast<float>(index / 2) * fieldRowHeight;
            ui.label({x, fieldY - 2}, field.label, theme.textMuted);
            const std::string id = std::format("inspector.{}.{}.{}", entity->id, card.title, field.label);
            if (auto committed = ui.textField(id, {x, fieldY + fieldLabelGap, columnWidth, fieldHeight}, field.value)) {
                field.apply(*committed);
                state.modified();
            }
        }
        y += box.h + cardGap;
    }

    const Rect addButton{content.x, y, content.w, 38};
    if (ui.button(addButton, "+ Adicionar componente", ButtonStyle::Dashed)) addMenuOpen = !addMenuOpen;
    ui.draw.popClip();

    if (addMenuOpen) drawAddMenu(ui, state, *entity, addButton);
    if (pendingRemoval) {
        pendingRemoval(*entity);
        state.modified();
    }
    const float contentHeight = y + addButton.h + scroll - content.y + menuItemHeight * 5;
    scroll = std::min(scroll, std::max(0.0f, contentHeight - content.h));
}

void Inspector::drawSceneSettings(Ui& ui, EditorState& state, Rect area) {
    const Theme& theme = ui.theme;
    Scene& scene = state.active();
    struct Option {
        CameraMode mode;
        std::string_view label;
        std::string_view description;
    };
    constexpr Option options[] = {
        {CameraMode::Side, "Plataforma", "lateral"},
        {CameraMode::TopDown, "RPG", "de cima"},
        {CameraMode::Isometric, "Isométrica", "losango"},
    };
    constexpr float optionHeight = 34;
    const Rect box{area.x, area.y, area.w, cardTitleHeight + 16 + optionHeight * 3 + 40};
    ui.panel(box, theme.card);
    ui.labelLeft({box.x + 10, box.y, box.w, cardTitleHeight}, "Cena", theme.text, TextStyle::Strong);
    ui.labelRight({box.x, box.y, box.w - 10, cardTitleHeight}, scene.name, theme.textMuted, TextStyle::Mono);
    ui.label({box.x + 10, box.y + cardTitleHeight - 4}, "Câmera", theme.textMuted);

    float y = box.y + cardTitleHeight + 16;
    for (const auto& option : options) {
        const Rect button{box.x + 10, y, box.w - 20, optionHeight - 4};
        const bool active = scene.camera == option.mode;
        if (ui.button(button, {}, active ? ButtonStyle::Toggled : ButtonStyle::Outlined) && !active && state.editing()) {
            scene.camera = option.mode;
            state.modified();
        }
        ui.labelLeft(button.inset(10), option.label, active ? theme.highlight : theme.text);
        ui.labelRight(button.inset(10), option.description, theme.textMuted);
        y += optionHeight;
    }
    const std::string_view hint = state.editing() ? "Selecione uma entidade." : "Pare o jogo para trocar.";
    ui.labelLeft({box.x + 10, box.bottom() - 32, box.w, 28}, hint, theme.textMuted);
}

float Inspector::drawEntityHeader(Ui& ui, EditorState& state, Entity& entity, Rect area) {
    const Theme& theme = ui.theme;
    Rect nameRow = area.takeTop(30);
    ui.swatch({nameRow.x + 6, nameRow.y + 15}, entityColor(entity, theme), 10);
    nameRow.takeLeft(18);
    const std::string idLabel = std::format("entity #{:04}", entity.id);
    const Rect idArea = nameRow.takeRight(ui.draw.textWidth(idLabel, TextStyle::Mono) + 4);
    ui.labelRight(idArea, idLabel, theme.textMuted, TextStyle::Mono);
    nameRow.takeRight(8);
    if (auto name = ui.textField(std::format("inspector.{}.name", entity.id), nameRow, entity.name)) {
        if (!name->empty()) entity.name = *name;
        state.modified();
    }

    area.takeTop(8);
    Rect groupRow = area.takeTop(30);
    ui.labelLeft(groupRow, "Grupo", theme.textMuted);
    groupRow.takeLeft(56);
    if (auto group = ui.textField(std::format("inspector.{}.group", entity.id), groupRow, entity.group, "(nenhum)")) {
        entity.group = *group;
        state.modified();
    }
    return 30 + 8 + 30 + 14;
}

void Inspector::drawAddMenu(Ui& ui, EditorState& state, Entity& entity, const Rect& anchor) {
    std::vector<AddOption> available;
    for (auto& option : addOptions()) {
        if (!option.present(entity)) available.push_back(std::move(option));
    }
    if (available.empty()) {
        addMenuOpen = false;
        return;
    }
    const Rect menu{anchor.x, anchor.bottom() + 4, anchor.w, menuItemHeight * static_cast<float>(available.size()) + 8};
    ui.beginPopup(menu);
    float y = menu.y + 4;
    for (const auto& option : available) {
        const Rect item{menu.x + 4, y, menu.w - 8, menuItemHeight};
        if (ui.hovered(item)) ui.draw.fill(item, ui.theme.accentSoft);
        ui.labelLeft(item.inset(10), option.name, ui.theme.text);
        if (ui.pressed(item)) {
            option.add(entity, state);
            state.modified();
            addMenuOpen = false;
        }
        y += menuItemHeight;
    }
    ui.endPopup();
    if (ui.input.mousePressed(MouseButton::Left) && !menu.contains(ui.input.mouse()) && !anchor.contains(ui.input.mouse())) {
        addMenuOpen = false;
    }
}

}
