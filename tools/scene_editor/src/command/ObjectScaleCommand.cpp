#include "ObjectScaleCommand.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>

#include "AxisFlags.h"
#include "Scene.h"
#include "input/InputHandler.h"
#include "scene/action/ScaleObject.h"



class ObjectScaleState : public InputFrame
{
public:
    ObjectScaleState(SceneObject obj, s_ptr<Scene> scene, const MouseState& cursor)
        :
        obj(obj),
        scene(scene),
        pivot(scene->get<ObjectBaseNode>(obj).getTranslation()),
        depth(scene->getCamera().calcScreenDepth(pivot)),
        originalCursorPosUL(cursor.getCursorPos()),
        originalScaling(scene->get<ObjectBaseNode>(obj).getScale())
    {
    }

    void updateScalingPreview(const CursorMovement& _cursor)
    {
        const auto& camera = scene->getCamera();
        const auto cursor = _cursor.invertY();

        const vec2 origCursor{ originalCursorPosUL.x, cursor.areaSize.y - originalCursorPosUL.y };
        const vec3 originalWorldPos = camera.unproject(origCursor, depth, cursor.areaSize);
        const vec3 newWorldPos = camera.unproject(cursor.position, depth, cursor.areaSize);

        // The factor is set up such that the distance from the pivot to the
        // original cursor position (when the command was started) represents
        // the scaling value range `[0, originalScaling]`; Moving the cursor
        // on top of the scaled object changes the scaling to 0, while moving it away
        // from the object makes it larger.
        const float factor = glm::distance(pivot, newWorldPos)
                             / glm::distance(pivot, originalWorldPos);
        const float sign = glm::sign(glm::dot(originalWorldPos - pivot, newWorldPos - pivot));

        newScaling = originalScaling * sign * factor * lockedAxis;

        scene->get<ObjectBaseNode>(obj).setScale(newScaling);
    }

    void applyScaling(CommandExecutionContext& ctx)
    {
        ctx.generateAction(
            std::make_unique<action::ScaleObject>(scene, obj, originalScaling, newScaling)
        );
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
    s_ptr<Scene> scene;

    const vec3 pivot;
    const float depth;
    const vec2 originalCursorPosUL;
    const vec3 originalScaling;

    vec3 lockedAxis{ 1, 1, 1 };
    vec3 newScaling{ originalScaling };
};



ObjectScaleCommand::ObjectScaleCommand(s_ptr<Scene> scene)
    :
    scene(scene)
{}

void ObjectScaleCommand::execute(CommandExecutionContext& ctx)
{
    scene->getSelectedObject() >> [&](auto obj)
    {
        auto state = ctx.pushFrame(ObjectScaleState{ obj, scene, ctx.mouse() });

        state.on(trc::Key::escape,        [](auto& state){ state.resetScaling(); });
        state.on(trc::MouseButton::right, [](auto& state){ state.resetScaling(); });
        state.on(trc::Key::enter,         [](auto& state, auto& ctx){ state.applyScaling(ctx); });
        state.on(trc::MouseButton::left,  [](auto& state, auto& ctx){ state.applyScaling(ctx); });

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
