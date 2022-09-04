#include "ObjectTranslateCommand.h"

#include "App.h"
#include "Scene.h"
#include "input/InputState.h"
#include "AxisFlags.h"



class ObjectTranslateState : public CommandState
{
public:
    ObjectTranslateState(SceneObject obj, Scene& scene)
        :
        obj(obj),
        scene(&scene),
        originalPos(scene.get<ObjectBaseNode>(obj).getTranslation()),
        originalMousePos(
            scene.getMousePosAtDepth(
                scene.getCamera().calcScreenDepth(scene.get<ObjectBaseNode>(obj).getTranslation())
            )
        ),
        finalPos(originalPos)
    {}

    bool update(float) override
    {
        scene->get<ObjectBaseNode>(obj).setTranslation(getNewPos());
        return terminate;
    }

    void onExit() override
    {
        scene->get<ObjectBaseNode>(obj).setTranslation(finalPos);
    }

    void applyPlacement()
    {
        finalPos = getNewPos();
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
    auto getNewPos() const -> vec3
    {
        const float depth = scene->getCamera().calcScreenDepth(
            scene->get<ObjectBaseNode>(obj).getTranslation()
        );
        const vec3 diff = scene->getMousePosAtDepth(depth) - originalMousePos;

        return originalPos + diff * lockedAxis;
    }

    const SceneObject obj;
    Scene* scene;

    const vec3 originalPos;
    const vec3 originalMousePos;
    vec3 finalPos;

    vec3 lockedAxis{ 1, 1, 1 };
    bool terminate{ false };
};

void ObjectTranslateCommand::execute(CommandCall& call)
{
    auto& scene = App::get().getScene();
    scene.getSelectedObject() >> [&](auto obj)
    {
        auto& state = call.setState(ObjectTranslateState{ obj, scene });

        call.on(vkb::Key::escape,        [&](auto&){ state.resetPlacement(); });
        call.on(vkb::MouseButton::right, [&](auto&){ state.resetPlacement(); });
        call.on(vkb::Key::enter,         [&](auto&){ state.applyPlacement(); });
        call.on(vkb::MouseButton::left,  [&](auto&){ state.applyPlacement(); });

        // x and y keys are swapped because it seems like glfw uses the american keyboard (why?)
        call.on({ vkb::Key::x }, [&](auto&){ state.lockAxes(Axis::eY | Axis::eZ); });
        call.on({ vkb::Key::z }, [&](auto&){ state.lockAxes(Axis::eX | Axis::eZ); });
        call.on({ vkb::Key::y }, [&](auto&){ state.lockAxes(Axis::eX | Axis::eY); });
        call.on({ vkb::Key::x, vkb::KeyModFlagBits::shift }, [&](auto&){ state.lockAxes(Axis::eX); });
        call.on({ vkb::Key::z, vkb::KeyModFlagBits::shift }, [&](auto&){ state.lockAxes(Axis::eY); });
        call.on({ vkb::Key::y, vkb::KeyModFlagBits::shift }, [&](auto&){ state.lockAxes(Axis::eZ); });
    };
}
