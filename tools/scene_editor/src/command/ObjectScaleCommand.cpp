#include "ObjectScaleCommand.h"

#include "AxisFlags.h"
#include "Globals.h"
#include "Scene.h"
#include "input/InputState.h"



class ObjectScaleState : public InputFrame
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
        originalScaling(scene.get<ObjectBaseNode>(obj).getScale()),
        finalScaling(originalScaling)
    {
    }

    void onTick(float) override {}

    void onExit() override
    {
        scene->get<ObjectBaseNode>(obj).setScale(finalScaling);
    }

    void updateScalingPreview()
    {
        scene->get<ObjectBaseNode>(obj).setScale(calcNewScaling());
    }

    void applyScaling()
    {
        finalScaling = calcNewScaling();
        exitFrame();
    }

    void resetScaling()
    {
        finalScaling = originalScaling;
        exitFrame();
    }

    void lockAxes(AxisFlags flags)
    {
        lockedAxis = toVector(flags);
    }

private:
    auto calcNewScaling() const -> vec3
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
};



void ObjectScaleCommand::execute(CommandExecutionContext& ctx)
{
    auto& scene = g::scene();
    scene.getSelectedObject() >> [&](auto obj)
    {
        auto state = ctx.pushFrame(std::make_unique<ObjectScaleState>(obj, scene));

        state.on(trc::Key::escape,        [](auto& state){ state.resetScaling(); });
        state.on(trc::MouseButton::right, [](auto& state){ state.resetScaling(); });
        state.on(trc::Key::enter,         [](auto& state){ state.applyScaling(); });
        state.on(trc::MouseButton::left,  [](auto& state){ state.applyScaling(); });

        // x and y keys are swapped because the key codes use the american keyboard
        state.on({ trc::Key::x }, [](auto& state){ state.lockAxes(Axis::eY | Axis::eZ); });
        state.on({ trc::Key::z }, [](auto& state){ state.lockAxes(Axis::eX | Axis::eZ); });
        state.on({ trc::Key::y }, [](auto& state){ state.lockAxes(Axis::eX | Axis::eY); });
        state.on({ trc::Key::x, trc::KeyModFlagBits::shift }, [](auto& state){ state.lockAxes(Axis::eX); });
        state.on({ trc::Key::z, trc::KeyModFlagBits::shift }, [](auto& state){ state.lockAxes(Axis::eY); });
        state.on({ trc::Key::y, trc::KeyModFlagBits::shift }, [](auto& state){ state.lockAxes(Axis::eZ); });

        state.onCursorMove([](auto& state, auto&&){ state.updateScalingPreview(); });
    };
}
