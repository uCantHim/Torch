#include "KeyConfig.h"

#include "Globals.h"
#include "command/CameraCommands.h"
#include "command/ObjectRotateCommand.h"
#include "command/ObjectScaleCommand.h"
#include "command/ObjectTranslateCommand.h"
#include "gui/ContextMenu.h"
#include "object/Context.h"



void openContextMenu(Scene& scene)
{
    scene.getHoveredObject() >> [&](SceneObject obj) {
        gui::ContextMenu::show("object " + obj.toString(), makeContext(scene, obj));
    };
}

void selectHoveredObject()
{
    g::scene().selectHoveredObject();
}

auto makeInputFrame(const KeyConfig& conf, App& app) -> u_ptr<InputFrame>
{
    struct DefaultRootInputFrame : InputFrame
    {
        void onTick(float) override {}
        void onExit() override {}
    };

    auto f = std::make_unique<DefaultRootInputFrame>();

    f->on(conf.closeApp,            [&]{ app.end(); });
    f->on(conf.openContext,         [&]{ openContextMenu(app.getScene()); });
    f->on(conf.selectHoveredObject, selectHoveredObject);
    f->on(conf.deleteHoveredObject, [&app]{
        app.getScene().getSelectedObject() >> [&](SceneObject obj) {
            app.getScene().deleteObject(obj);
        };
    });

    f->on(conf.cameraRotate, std::make_unique<CameraRotateCommand>(app));
    f->on(conf.cameraMove,   std::make_unique<CameraMoveCommand>(app));

    f->on(conf.translateObject, std::make_unique<ObjectTranslateCommand>(app));
    f->on(conf.scaleObject,     std::make_unique<ObjectScaleCommand>());
    f->on(conf.rotateObject,    std::make_unique<ObjectRotateCommand>());

    f->onScroll([&app, scrollLevel=0](auto&, const Scroll& scroll) mutable {
        scrollLevel += static_cast<i32>(glm::sign(scroll.offset.y));
        app.getScene().getCameraArm().setZoomLevel(scrollLevel);
    });

    f->onUnhandledMouseInput([contextMenuKey=conf.openContext](auto&, MouseInput input) {
        std::cout << "unhandled mouse input\n";
        if (input.action == trc::InputAction::press && input != contextMenuKey) {
            std::cout << "    closing context menu.\n";
            gui::ContextMenu::close();
        }
    });

    return f;
}
