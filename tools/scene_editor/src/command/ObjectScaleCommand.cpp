#include "ObjectScaleCommand.h"

#include "App.h"
#include "Scene.h"
#include "input/InputState.h"
#include "AxisFlags.h"



class ObjectScaleState : public CommandState
{
public:
    ObjectScaleState(SceneObject obj, Scene& scene)
        :
        obj(obj),
        scene(&scene),
        pivot(scene.get<ObjectBaseNode>(obj).getTranslation()),
        depth(scene.getCamera().calcScreenDepth(pivot)),
        // We divide by the original pivot distance later on, so it cannot be zero
        originalPivotDist(glm::max(0.001f, glm::distance(pivot, scene.getMousePosAtDepth(depth)))),
        originalScaling(scene.get<ObjectBaseNode>(obj).getScale())
    {
    }

    bool update(const float) override
    {
        scene->get<ObjectBaseNode>(obj).setScale(getNewScaling());

        return terminate;
    }

    void onExit() override
    {
        scene->get<ObjectBaseNode>(obj).setScale(finalScaling);
    }

    void applyScaling()
    {
        finalScaling = getNewScaling();
        terminate = true;
    }

    void resetScaling()
    {
        finalScaling = originalScaling;
        terminate = true;
    }

    void lockAxes(AxisFlags flags)
    {
        lockedAxis = toVector(flags);
    }

private:
    auto getNewScaling() const -> vec3
    {
        const float dist = glm::distance(pivot, scene->getMousePosAtDepth(depth));

        assert(originalPivotDist > 0.0f);
        return originalScaling + ((dist / originalPivotDist) - 1.0f) * lockedAxis;
    }

    const SceneObject obj;
    Scene* scene;

    const vec3 pivot;
    const float depth;
    const float originalPivotDist;
    const vec3 originalScaling;
    vec3 finalScaling;

    vec3 lockedAxis{ 1, 1, 1 };
    bool terminate{ false };
};



void ObjectScaleCommand::execute(CommandCall& call)
{
    auto& scene = App::get().getScene();
    scene.getSelectedObject() >> [&](auto obj)
    {
        auto& state = call.setState(ObjectScaleState{ obj, scene });

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
