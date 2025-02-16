#include "ObjectRotateCommand.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/vec_swizzle.hpp>

#include "AxisFlags.h"
#include "Scene.h"
#include "input/InputHandler.h"
#include "scene/action/RotateObject.h"



class ObjectRotateState : public InputFrame
{
public:
    ObjectRotateState(SceneObject obj, s_ptr<Scene> scene)
        :
        obj(obj),
        scene(scene),
        originalOrientation(scene->get<ObjectBaseNode>(obj).getRotation()),
        pivotWorld(scene->get<ObjectBaseNode>(obj).getGlobalTransform()[3]),
        depth(scene->getCamera().calcScreenDepth(pivotWorld)),
        orientation(1, 0, 0, 0)
    {
    }

    void applyRotation(CommandExecutionContext& ctx)
    {
        ctx.generateAction(
            std::make_unique<action::RotateObject>(scene, obj, originalOrientation, orientation)
        );
        exitFrame();
    }

    void resetRotation()
    {
        scene->get<ObjectBaseNode>(obj).setRotation(originalOrientation);
        exitFrame();
    }

    void lockAxes(AxisFlags flags)
    {
        axisLock = toVector(flags);
        orientation = quat{ 1, 0, 0, 0 };
        scene->get<ObjectBaseNode>(obj).setRotation(originalOrientation);
    }

    void handleCursorMove(const CursorMovement& cursor)
    {
        const auto [angle, axis] = calcRotation(cursor);
        orientation = glm::rotate(orientation, angle, axis);
        scene->get<ObjectBaseNode>(obj).setRotation(originalOrientation * orientation);
    }

    auto calcRotation(const CursorMovement& _cursor) const -> std::pair<float, vec3>
    {
        /** Component-wise sign. Returns 1 if x > 0, 1 if x == 0, -1 if x < 0. */
        constexpr auto sign_nonnull = [](vec3 v) -> vec3 {
            return { v.x >= 0.0f ? 1 : -1, v.y >= 0.0f ? 1 : -1, v.z >= 0.0f ? 1 : -1, };
        };

        // Invert y-axis on cursor coordinates (origin needs to be lower left)
        const CursorMovement cursor = _cursor.invertY();
        const vec2 cursorPos = cursor.position;
        const vec2 prevCursorPos = cursorPos - cursor.offset;

        // Compute screen space vectors
        const vec4 pivotProj = scene->getCamera().project(pivotWorld);
        const vec2 pivot = glm::xy(pivotProj) * vec2{cursor.areaSize};

        // Compute angle and direction
        const vec2 a = cursorPos - pivot;
        const vec2 b = prevCursorPos - pivot;
        const float angle = glm::acos(glm::dot(glm::normalize(a), glm::normalize(b)));
        const float sign = glm::sign(a.x * b.y - b.x * a.y);  // Derived from cross product on
                                                              // x-y plane

        // Compute rotation axis
        const vec3 eyeVec = pivotWorld - scene->getCameraArm().getCameraWorldPos();
        const vec3 axis = axisLock ? (*axisLock * sign_nonnull(eyeVec))
                                   : glm::normalize(eyeVec);

        return { sign * (glm::isnan(angle) ? 0.0f : angle), axis };
    }

    const SceneObject obj;
    s_ptr<Scene> scene;

    const quat originalOrientation;
    const vec3 pivotWorld;
    const float depth;

    std::optional<vec3> axisLock;
    quat orientation;
};



ObjectRotateCommand::ObjectRotateCommand(s_ptr<Scene> scene)
    :
    scene(scene)
{}

void ObjectRotateCommand::execute(CommandExecutionContext& ctx)
{
    scene->getSelectedObject() >> [&](auto obj)
    {
        using S = ObjectRotateState;
        auto state = ctx.pushFrame(ObjectRotateState{ obj, scene });

        state.on(trc::Key::escape,        [](S& state){ state.resetRotation(); });
        state.on(trc::MouseButton::right, [](S& state){ state.resetRotation(); });
        state.on(trc::Key::enter,         [](S& state, auto& ctx){ state.applyRotation(ctx); });
        state.on(trc::MouseButton::left,  [](S& state, auto& ctx){ state.applyRotation(ctx); });

        // x and y keys are swapped because key codes use the american keyboard layout
        auto shift = trc::KeyModFlagBits::shift;
        state.on({ trc::Key::x }, [](S& state){ state.lockAxes(Axis::eY | Axis::eZ); });
        state.on({ trc::Key::z }, [](S& state){ state.lockAxes(Axis::eX | Axis::eZ); });
        state.on({ trc::Key::y }, [](S& state){ state.lockAxes(Axis::eX | Axis::eY); });
        state.on({ trc::Key::x, shift }, [](S& state){ state.lockAxes(Axis::eY | Axis::eZ); });
        state.on({ trc::Key::z, shift }, [](S& state){ state.lockAxes(Axis::eX | Axis::eZ); });
        state.on({ trc::Key::y, shift }, [](S& state){ state.lockAxes(Axis::eX | Axis::eY); });

        state.onCursorMove(&ObjectRotateState::handleCursorMove);
    };
}
