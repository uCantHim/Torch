#include "KeyConfig.h"

#include "App.h"
#include "gui/ContextMenu.h"
#include "command/ObjectTranslateCommand.h"
#include "command/ObjectScaleCommand.h"
#include "command/ObjectRotateCommand.h"



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
    map.set(conf.scaleObject,     std::make_unique<ObjectScaleCommand>());
    map.set(conf.rotateObject,    std::make_unique<ObjectRotateCommand>());

    // General stuff that I don't how to deal with
    trc::on<trc::MouseClickEvent>([](auto&) { gui::ContextMenu::close(); });

    return map;
}
