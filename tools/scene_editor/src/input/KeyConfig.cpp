#include "KeyConfig.h"

#include "App.h"
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

void setupMainSceneInputFrame(InputFrame& f, const KeyConfig& conf, s_ptr<Scene> scene)
{
    f.on(conf.openContext,         [scene]{ openContextMenu(*scene); });
    f.on(conf.selectHoveredObject, selectHoveredObject);
    f.on(conf.deleteHoveredObject, [scene]{
        scene->getSelectedObject() >> [&](SceneObject obj) {
            scene->deleteObject(obj);
        };
    });

    f.on(conf.cameraRotate, std::make_unique<CameraRotateCommand>(scene));
    f.on(conf.cameraMove,   std::make_unique<CameraMoveCommand>(scene));

    f.on(conf.translateObject, std::make_unique<ObjectTranslateCommand>(scene));
    f.on(conf.scaleObject,     std::make_unique<ObjectScaleCommand>(scene));
    f.on(conf.rotateObject,    std::make_unique<ObjectRotateCommand>(scene));

    f.onScroll([scene, scrollLevel=0](auto&, const Scroll& scroll) mutable {
        scrollLevel += static_cast<i32>(glm::sign(scroll.offset.y));
        scene->getCameraArm().setZoomLevel(scrollLevel);
    });

    f.onCursorMove([scene](auto&, const CursorMovement& cursor) {
        scene->notifyCursorMove(cursor);
    });

    f.onUnhandledMouseInput([contextMenuKey=conf.openContext](auto&, MouseInput input) {
        if (input.action == trc::InputAction::press && input != contextMenuKey) {
            gui::ContextMenu::close();
        }
    });
}
