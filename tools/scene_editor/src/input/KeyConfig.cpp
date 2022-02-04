#include "KeyConfig.h"

#include "App.h"
#include "gui/ContextMenu.h"



void openContextMenu(App& app)
{
    app.getScene().openContextMenu();
}

auto makeKeyMap(App& app, const KeyConfig& conf) -> KeyMap
{
    KeyMap map;

    map.set(conf.closeApp,    makeInputCommand([&]{ app.end(); }));
    map.set(conf.openContext, makeInputCommand(openContextMenu));

    // General stuff that I don't how to deal with
    vkb::on<vkb::MouseClickEvent>([](auto&) { gui::ContextMenu::close(); });

    return map;
}
