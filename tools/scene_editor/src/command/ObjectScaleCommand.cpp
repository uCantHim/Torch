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
        originalScaling(scene.get<ObjectBaseNode>(obj).getScale())
    {
    }

    void updateScalingPreview(const CursorMovement& cursor)
    {
        assert(originalPivotDist > 0.0f);

        const vec3 worldPos = scene->getCamera().unproject(cursor.position, depth, cursor.areaSize);
        const float dist = glm::distance(pivot, worldPos);
        const vec3 s = originalScaling + ((dist / originalPivotDist) - 1.0f) * lockedAxis;

        scene->get<ObjectBaseNode>(obj).setScale(s);
    }

    void applyScaling()
    {
        exitFrame();
    }

    void resetScaling()
    {
        scene->get<ObjectBaseNode>(obj).setScale(originalScaling);
        exitFrame();
    }

    void lockAxes(AxisFlags flags)
    {
        lockedAxis = toVector(flags);
    }

private:
    const SceneObject obj;
    Scene* scene;

    const vec3 pivot;
    const float depth;
    const float originalPivotDist;
    const vec3 originalScaling;

    vec3 lockedAxis{ 1, 1, 1 };
};



void ObjectScaleCommand::execute(CommandExecutionContext& ctx)
{
    auto& scene = g::scene();
    scene.getSelectedObject() >> [&](auto obj)
    {
        auto state = ctx.pushFrame(ObjectScaleState{ obj, scene });

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

        state.onCursorMove([](auto& state, auto&& cursor){
            state.updateScalingPreview(cursor);
        });
    };
}
