#include "ObjectTranslateCommand.h"

#include <trc/base/event/InputState.h>

#include "AxisFlags.h"
#include "Globals.h"
#include "Scene.h"
#include "input/InputState.h"



class ObjectTranslateState : public InputFrame
{
public:
    ObjectTranslateState(SceneObject obj, Scene& _scene)
        :
        obj(obj),
        scene(&_scene),
        originalPos(_scene.get<ObjectBaseNode>(obj).getTranslation())
    {}

    void onMouseMove(const CursorMovement& cursor)
    {
        const vec2 windowSize = g::torch().getWindow().getWindowSize();
        const auto diff = cursor.offset / windowSize * kDragSpeed;

        const auto& camera = scene->getCamera();
        const vec3 worldDiff = glm::inverse(camera.getViewMatrix())
                             * glm::inverse(camera.getProjectionMatrix())
                             * vec4(diff.x, -diff.y, 0, 0);

        scene->get<ObjectBaseNode>(obj).translate(worldDiff * lockedAxis);
    }

    void applyPlacement()
    {
        exitFrame();
    }

    void resetPlacement()
    {
        scene->get<ObjectBaseNode>(obj).setTranslation(originalPos);
        exitFrame();
    }

    void lockAxes(AxisFlags axes)
    {
        scene->get<ObjectBaseNode>(obj).setTranslation(originalPos);
        lockedAxis = toVector(axes);
    }

private:
    static constexpr float kDragSpeed{ 15.0f };

    const SceneObject obj;
    Scene* scene;

    const vec3 originalPos;
    vec3 lockedAxis{ 1, 1, 1 };
};



void ObjectTranslateCommand::execute(CommandExecutionContext& ctx)
{
    auto& scene = g::scene();
    scene.getSelectedObject() >> [&](auto obj)
    {
        auto state = ctx.pushFrame(ObjectTranslateState{ obj, scene });

        state.on(trc::Key::escape,        [&](auto& state){ state.resetPlacement(); });
        state.on(trc::MouseButton::right, [&](auto& state){ state.resetPlacement(); });
        state.on(trc::Key::enter,         [&](auto& state){ state.applyPlacement(); });
        state.on(trc::MouseButton::left,  [&](auto& state){ state.applyPlacement(); });

        // x and y keys are swapped because it seems like glfw uses the american keyboard (why?)
        state.on({ trc::Key::x }, [&](auto& state){ state.lockAxes(Axis::eY | Axis::eZ); });
        state.on({ trc::Key::z }, [&](auto& state){ state.lockAxes(Axis::eX | Axis::eZ); });
        state.on({ trc::Key::y }, [&](auto& state){ state.lockAxes(Axis::eX | Axis::eY); });
        state.on({ trc::Key::x, trc::KeyModFlagBits::shift }, [&](auto& state){ state.lockAxes(Axis::eX); });
        state.on({ trc::Key::z, trc::KeyModFlagBits::shift }, [&](auto& state){ state.lockAxes(Axis::eY); });
        state.on({ trc::Key::y, trc::KeyModFlagBits::shift }, [&](auto& state){ state.lockAxes(Axis::eZ); });

        state.onCursorMove([](auto& state, auto&& cursor){ state.onMouseMove(cursor); });
    };
}
