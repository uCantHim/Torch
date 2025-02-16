#include "ObjectTranslateCommand.h"

#include <trc/base/event/InputState.h>

#include "AxisFlags.h"
#include "Scene.h"
#include "input/InputHandler.h"
#include "scene/action/MoveObject.h"



class ObjectTranslateState : public InputFrame
{
public:
    ObjectTranslateState(SceneObject obj, s_ptr<Scene> _scene)
        :
        obj(obj),
        scene(_scene),
        originalPos(_scene->get<ObjectBaseNode>(obj).getGlobalTransform()[3]),
        newPos(originalPos)
    {}

    void onMouseMove(const CursorMovement& _cursor)
    {
        const auto& camera = scene->getCamera();
        const auto cursor = _cursor.invertY();

        const float depth = camera.calcScreenDepth(originalPos);
        const vec3 a = camera.unproject(cursor.position - cursor.offset, depth, cursor.areaSize);
        const vec3 b = camera.unproject(cursor.position,                 depth, cursor.areaSize);

        newPos += (b - a) * lockedAxis;
        scene->get<ObjectBaseNode>(obj).setTranslation(newPos);
    }

    void applyPlacement(CommandExecutionContext& ctx)
    {
        ctx.generateAction(std::make_unique<action::MoveObject>(scene, obj, originalPos, newPos));
        exitFrame();
    }

    void resetPlacement()
    {
        scene->get<ObjectBaseNode>(obj).setTranslation(originalPos);
        exitFrame();
    }

    void lockAxes(AxisFlags axes)
    {
        lockedAxis = toVector(axes);
        newPos = originalPos + (newPos - originalPos) * lockedAxis;
        scene->get<ObjectBaseNode>(obj).setTranslation(newPos);
    }

private:
    const SceneObject obj;
    const s_ptr<Scene> scene;

    const vec3 originalPos;
    vec3 newPos;
    vec3 lockedAxis{ 1, 1, 1 };
};



ObjectTranslateCommand::ObjectTranslateCommand(s_ptr<Scene> scene)
    :
    scene(scene)
{}

void ObjectTranslateCommand::execute(CommandExecutionContext& ctx)
{
    scene->getSelectedObject() >> [&](auto obj)
    {
        auto state = ctx.pushFrame(ObjectTranslateState{ obj, scene });

        state.on(trc::Key::escape,        [](auto& state){ state.resetPlacement(); });
        state.on(trc::MouseButton::right, [](auto& state){ state.resetPlacement(); });
        state.on(trc::Key::enter,         [](auto& state, auto&& ctx){ state.applyPlacement(ctx); });
        state.on(trc::MouseButton::left,  [](auto& state, auto&& ctx){ state.applyPlacement(ctx); });

        // x and y keys are swapped because key codes use american layout
        state.on(trc::Key::x, [](auto& state){ state.lockAxes(Axis::eY | Axis::eZ); });
        state.on(trc::Key::z, [](auto& state){ state.lockAxes(Axis::eX | Axis::eZ); });
        state.on(trc::Key::y, [](auto& state){ state.lockAxes(Axis::eX | Axis::eY); });
        state.on({ trc::Key::x, trc::KeyModFlagBits::shift }, [](auto& state){ state.lockAxes(Axis::eX); });
        state.on({ trc::Key::z, trc::KeyModFlagBits::shift }, [](auto& state){ state.lockAxes(Axis::eY); });
        state.on({ trc::Key::y, trc::KeyModFlagBits::shift }, [](auto& state){ state.lockAxes(Axis::eZ); });

        state.onCursorMove([](auto& state, auto&& cursor){ state.onMouseMove(cursor); });
    };
}
