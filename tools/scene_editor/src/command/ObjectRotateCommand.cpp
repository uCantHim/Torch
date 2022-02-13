#include "ObjectRotateCommand.h"

#include "App.h"
#include "Scene.h"
#include "input/InputState.h"
#include "AxisFlags.h"


class ObjectRotateState : public CommandState
{
public:
    ObjectRotateState(SceneObject obj, Scene& scene)
        :
        obj(obj),
        scene(&scene)
    {
    }

    bool update(const float) override
    {
        return true;
    }

    void onExit() override
    {
    }

    void applyScaling()
    {
    }

    void resetScaling()
    {
    }

    void lockAxes(AxisFlags flags)
    {
        rotationAxis = toVector(flags);
    }

private:
    const SceneObject obj;
    Scene* scene;

    vec3 rotationAxis{ 1, 1, 1 };
};



void ObjectRotateCommand::execute(CommandCall& call)
{
    auto& scene = App::get().getScene();
    scene.getSelectedObject() >> [&](auto obj)
    {
        auto& state = call.setState(ObjectRotateState{ obj, scene });

        call.on(vkb::Key::escape,        [&](auto&){ state.resetScaling(); });
        call.on(vkb::MouseButton::right, [&](auto&){ state.resetScaling(); });
        call.on(vkb::Key::enter,         [&](auto&){ state.applyScaling(); });
        call.on(vkb::MouseButton::left,  [&](auto&){ state.applyScaling(); });

        // x and y keys are swapped because it seems like glfw uses the american keyboard (why?)
        call.on({ vkb::Key::x }, [&](auto&){ state.lockAxes(Axis::eY | Axis::eZ); });
        call.on({ vkb::Key::z }, [&](auto&){ state.lockAxes(Axis::eX | Axis::eZ); });
        call.on({ vkb::Key::y }, [&](auto&){ state.lockAxes(Axis::eX | Axis::eY); });
        call.on({ vkb::Key::x, vkb::KeyModFlagBits::shift }, [&](auto&){ state.lockAxes(Axis::eX); });
        call.on({ vkb::Key::z, vkb::KeyModFlagBits::shift }, [&](auto&){ state.lockAxes(Axis::eY); });
        call.on({ vkb::Key::y, vkb::KeyModFlagBits::shift }, [&](auto&){ state.lockAxes(Axis::eZ); });
    };
}
