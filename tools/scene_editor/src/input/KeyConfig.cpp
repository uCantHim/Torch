#include "KeyConfig.h"

#include "App.h"
#include "gui/ContextMenu.h"
#include "command/ObjectTranslateCommand.h"



void openContextMenu()
{
    App::get().getScene().openContextMenu();
}

void selectHoveredObject()
{
    App::get().getScene().selectHoveredObject();
}

auto makeKeyMap(App& app, const KeyConfig& conf) -> KeyMap
{
    KeyMap map;

    map.set(conf.closeApp,            makeInputCommand([&]{ app.end(); }));
    map.set(conf.openContext,         makeInputCommand(openContextMenu));
    map.set(conf.selectHoveredObject, makeInputCommand(selectHoveredObject));
    map.set(conf.translateObject, std::make_unique<ObjectTranslateCommand>());

    // General stuff that I don't how to deal with
    vkb::on<vkb::MouseClickEvent>([](auto&) { gui::ContextMenu::close(); });

    return map;
}
