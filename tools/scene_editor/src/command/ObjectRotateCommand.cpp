#include "ObjectRotateCommand.h"

#include "AxisFlags.h"
#include "Globals.h"
#include "Scene.h"
#include "input/InputState.h"



class ObjectRotateState : public InputFrame
{
public:
    ObjectRotateState(SceneObject obj, Scene& scene)
        :
        obj(obj),
        scene(&scene),
        originalOrientation(scene.get<ObjectBaseNode>(obj).getRotation()),
        pivot(scene.get<ObjectBaseNode>(obj).getGlobalTransform()[3]),
        depth(scene.getCamera().calcScreenDepth(pivot)),
        originalDir(glm::normalize(scene.getMousePosAtDepth(depth) - pivot))
        //rotationAxis(glm::normalize(pivot - scene.getCameraArm().getCameraWorldPos()))
    {
        assert(glm::all(glm::not_(glm::isnan(originalDir))));
        //assert(glm::all(glm::not_(glm::isnan(rotationAxis))));
        assert(glm::all(glm::not_(glm::isnan(pivot))));
    }

    void applyRotation()
    {
        exitFrame();
    }

    void resetRotation()
    {
        scene->get<ObjectBaseNode>(obj).setRotation(originalOrientation);
        exitFrame();
    }

    void lockAxes(AxisFlags flags)
    {
        assert(glm::length(toVector(flags)) > 0.0f);
        axisLock = toVector(flags);
        scene->get<ObjectBaseNode>(obj).setRotation(originalOrientation);
    }

    void calcRotation(const CursorMovement& cursor)
    {
        assert(cursor.offset != vec2{ 0.0f });

        const vec2 cursorPos = cursor.position;
        const vec2 prevCursorPos = cursor.position - cursor.offset;

        auto& cam = scene->getCamera();
        const vec2 _pivot = cam.getProjectionMatrix() * cam.getViewMatrix() * vec4{pivot, 1};
        //_pivot = _pivot * viewportSize;

        std::cout << "Mouse: (" << cursorPos.x << ", " << cursorPos.y << ")\n"
                  << "Pivot: (" << _pivot.x << ", " << _pivot.y << ")\n";

        /*
        const vec3 dir = scene->unprojectScreenCoords(cursorPos, depth) - pivot;
        const vec3 prevDir = scene->unprojectScreenCoords(prevCursorPos, depth) - pivot;
        if (glm::length(dir) == 0.0f || glm::length(prevDir) == 0.0f) {
            return;
        }

        const vec3 a = glm::normalize(dir);
        const vec3 b = glm::normalize(prevDir);
        if (a == b) {
            return;
        }

        const vec3 axis = axisLock ? (*axisLock * glm::sign(glm::cross(a, b)))
                                   : glm::normalize(glm::cross(a, b));
        const float angle = glm::acos(glm::dot(a, b));
        */

        const vec2 a = cursorPos - _pivot;
        const vec2 b = prevCursorPos - _pivot;

        const vec3 axis = axisLock ? *axisLock
                                   : glm::normalize(scene->getCameraArm().getCameraWorldPos() - pivot);
        const float angle = glm::acos(glm::dot(glm::normalize(a), glm::normalize(b)));

        std::cout << "A: (" << a.x << ", " << a.y << ")\nB: (" << b.x << ", " << b.y << ")\n";
        const float sign = glm::sign(glm::cross(vec3(a, 0), vec3(b, 0)).z);
        std::cout << "Sign: " << sign << "\n";

        if (!glm::isnan(angle)) {
            scene->get<ObjectBaseNode>(obj).rotate(sign * angle, axis);
        }
        else {
            static int i = 0;
            std::cout << "NaN " << i++ << "\n";
        }
    }

    auto calcRotationAngle() const -> float
    {
        vec3 dir = scene->getMousePosAtDepth(depth) - pivot;
        if (float len = glm::length(dir); len > 0.0f) {
            dir /= len;
        }
        else {
            return 0.0f;
        }

        const vec3 cross = glm::cross(dir, originalDir);
        const float diff = glm::dot(dir, originalDir);
        if (glm::abs(diff) == 1.0f) {
            return 0.0f;
        }

        return -glm::sign(cross.x + cross.y + cross.z) * glm::acos(diff);
    }

    const SceneObject obj;
    Scene* scene;

    const quat originalOrientation;
    const vec3 pivot;
    const float depth;
    const vec3 originalDir;

    std::optional<vec3> axisLock;
};



void ObjectRotateCommand::execute(CommandExecutionContext& ctx)
{
    auto& scene = g::scene();
    scene.getSelectedObject() >> [&](auto obj)
    {
        auto state = ctx.pushFrame(ObjectRotateState{ obj, scene });

        state.on(trc::Key::escape,        [&](auto& state){ state.resetRotation(); });
        state.on(trc::MouseButton::right, [&](auto& state){ state.resetRotation(); });
        state.on(trc::Key::enter,         [&](auto& state){ state.applyRotation(); });
        state.on(trc::MouseButton::left,  [&](auto& state){ state.applyRotation(); });

        // x and y keys are swapped because key codes use the american keyboard layout
        auto shift = trc::KeyModFlagBits::shift;
        state.on({ trc::Key::x }, [&](auto& state){ state.lockAxes(Axis::eY | Axis::eZ); });
        state.on({ trc::Key::z }, [&](auto& state){ state.lockAxes(Axis::eX | Axis::eZ); });
        state.on({ trc::Key::y }, [&](auto& state){ state.lockAxes(Axis::eX | Axis::eY); });
        state.on({ trc::Key::x, shift }, [&](auto& state){ state.lockAxes(Axis::eY | Axis::eZ); });
        state.on({ trc::Key::z, shift }, [&](auto& state){ state.lockAxes(Axis::eX | Axis::eZ); });
        state.on({ trc::Key::y, shift }, [&](auto& state){ state.lockAxes(Axis::eX | Axis::eY); });

        state.onCursorMove([](ObjectRotateState& state, const CursorMovement& cursor){
            state.calcRotation(cursor);
            //state.setObjectRotation(state.calcRotationAngle());
        });
    };
}
