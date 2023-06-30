#include "ObjectTranslateCommand.h"

#include "AxisFlags.h"
#include "Globals.h"
#include "Scene.h"
#include "input/InputState.h"



class ObjectTranslateState : public CommandState
{
public:
    ObjectTranslateState(SceneObject obj)
        :
        obj(obj),
        scene(&g::scene()),
        originalPos(scene->get<ObjectBaseNode>(obj).getTranslation()),
        finalPos(originalPos)
    {}

    bool update(float) override
    {
        const auto now = trc::Mouse::getPosition();
        const vec2 windowSize = g::torch().getWindow().getWindowSize();
        const auto diff = (now - originalMousePos) / windowSize * kDragSpeed;

        const auto& camera = scene->getCamera();
        const vec3 worldDiff = glm::inverse(camera.getViewMatrix()) * vec4(diff.x, -diff.y, 0, 0);
        finalPos = originalPos + worldDiff * lockedAxis;

        scene->get<ObjectBaseNode>(obj).setTranslation(finalPos);

        return terminate;
    }

    void onExit() override
    {
        scene->get<ObjectBaseNode>(obj).setTranslation(finalPos);
    }

    void applyPlacement()
    {
        terminate = true;
    }

    void resetPlacement()
    {
        finalPos = originalPos;
        terminate = true;
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
    const vec2 originalMousePos{ trc::Mouse::getPosition() };
    vec3 finalPos;

    vec3 lockedAxis{ 1, 1, 1 };
    bool terminate{ false };
};

ObjectTranslateCommand::ObjectTranslateCommand(App& app)
    : app(&app)
{
}

void ObjectTranslateCommand::execute(CommandCall& call)
{
    auto& scene = g::scene();
    scene.getSelectedObject() >> [&](auto obj)
    {
        auto& state = call.setState(ObjectTranslateState{ obj });

        call.on(trc::Key::escape,        [&](auto&){ state.resetPlacement(); });
        call.on(trc::MouseButton::right, [&](auto&){ state.resetPlacement(); });
        call.on(trc::Key::enter,         [&](auto&){ state.applyPlacement(); });
        call.on(trc::MouseButton::left,  [&](auto&){ state.applyPlacement(); });

        // x and y keys are swapped because it seems like glfw uses the american keyboard (why?)
        call.on({ trc::Key::x }, [&](auto&){ state.lockAxes(Axis::eY | Axis::eZ); });
        call.on({ trc::Key::z }, [&](auto&){ state.lockAxes(Axis::eX | Axis::eZ); });
        call.on({ trc::Key::y }, [&](auto&){ state.lockAxes(Axis::eX | Axis::eY); });
        call.on({ trc::Key::x, trc::KeyModFlagBits::shift }, [&](auto&){ state.lockAxes(Axis::eX); });
        call.on({ trc::Key::z, trc::KeyModFlagBits::shift }, [&](auto&){ state.lockAxes(Axis::eY); });
        call.on({ trc::Key::y, trc::KeyModFlagBits::shift }, [&](auto&){ state.lockAxes(Axis::eZ); });
    };
}
