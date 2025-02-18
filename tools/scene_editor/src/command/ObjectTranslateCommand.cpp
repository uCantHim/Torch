#include "ObjectTranslateCommand.h"

#include <trc/base/event/InputState.h>

#include "AxisFlags.h"
#include "Globals.h"
#include "Scene.h"
#include "input/InputState.h"



class ObjectTranslateState : public InputFrame
{
public:
    ObjectTranslateState(SceneObject obj)
        :
        obj(obj),
        scene(&g::scene()),
        originalPos(scene->get<ObjectBaseNode>(obj).getTranslation()),
        finalPos(originalPos)
    {}

    void onTick(float) override {}

    void onExit() override
    {
        scene->get<ObjectBaseNode>(obj).setTranslation(finalPos);
    }

    void onMouseMove(const CursorMovement& cursor)
    {
        totalCursorMovement += cursor.offset;
        const vec2 windowSize = g::torch().getWindow().getWindowSize();
        const auto diff = totalCursorMovement / windowSize * kDragSpeed;

        const auto& camera = scene->getCamera();
        const vec3 worldDiff = glm::inverse(camera.getViewMatrix()) * vec4(diff.x, -diff.y, 0, 0);
        newPos = originalPos + worldDiff * lockedAxis;

        scene->get<ObjectBaseNode>(obj).setTranslation(newPos);
    }

    void applyPlacement()
    {
        finalPos = newPos;
        exitFrame();
    }

    void resetPlacement()
    {
        exitFrame();
    }

    void lockAxes(AxisFlags axes)
    {
        lockedAxis = vec3(!(axes & Axis::eX), !(axes & Axis::eY), !(axes & Axis::eZ));
    }

private:
    static constexpr float kDragSpeed{ 10.0f };

    const SceneObject obj;
    Scene* scene;

    const vec3 originalPos;
    vec2 totalCursorMovement;
    vec3 newPos;
    vec3 finalPos;

    vec3 lockedAxis{ 1, 1, 1 };
};

ObjectTranslateCommand::ObjectTranslateCommand(App& app)
    : app(&app)
{
}

void ObjectTranslateCommand::execute(CommandExecutionContext& ctx)
{
    auto& scene = g::scene();
    scene.getSelectedObject() >> [&](auto obj)
    {
        auto state = ctx.pushFrame(std::make_unique<ObjectTranslateState>(obj));

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

        //state.onCursorMove(&ObjectTranslateState::onMouseMove);
        state.onCursorMove([](auto& state, auto&& cursor){ state.onMouseMove(cursor); });
    };
}
