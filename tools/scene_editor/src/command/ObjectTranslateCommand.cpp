#include "ObjectTranslateCommand.h"

#include <trc/base/event/InputState.h>

#include "AxisFlags.h"
#include "Globals.h"
#include "Scene.h"
#include "input/InputState.h"



class MoveObject : public InvertibleAction
{
public:
    MoveObject(Scene* scene, SceneObject obj, vec3 from, vec3 to)
        : scene(scene), obj(obj), from(from), to(to)
    {}

    void apply() override {
        scene->get<ObjectBaseNode>(obj).setTranslation(to);
    }

    void undo() override {
        scene->get<ObjectBaseNode>(obj).setTranslation(from);
    }

    Scene* scene;
    const SceneObject obj;
    const vec3 from;
    const vec3 to;
};

class ObjectTranslateState : public InputFrame
{
public:
    ObjectTranslateState(SceneObject obj, Scene& _scene)
        :
        obj(obj),
        scene(&_scene),
        originalPos(_scene.get<ObjectBaseNode>(obj).getTranslation()),
        newPos(originalPos)
    {}

    void onMouseMove(const CursorMovement& cursor)
    {
        const auto diff = cursor.offset / vec2{cursor.areaSize} * kDragSpeed;

        const auto& camera = scene->getCamera();
        const vec3 worldDiff = glm::inverse(camera.getViewMatrix())
                             * glm::inverse(camera.getProjectionMatrix())
                             * vec4(diff.x, -diff.y, 0, 0);

        newPos += worldDiff * lockedAxis;
        scene->get<ObjectBaseNode>(obj).setTranslation(newPos);
    }

    void applyPlacement(CommandExecutionContext& ctx)
    {
        ctx.applyAction(std::make_unique<MoveObject>(scene, obj, originalPos, newPos));
        exitFrame();
    }

    void resetPlacement()
    {
        scene->get<ObjectBaseNode>(obj).setTranslation(originalPos);
        exitFrame();
    }

    void lockAxes(AxisFlags axes)
    {
        scene->get<ObjectBaseNode>(obj).setTranslation(originalPos);
        lockedAxis = toVector(axes);
    }

private:
    static constexpr float kDragSpeed{ 15.0f };

    const SceneObject obj;
    Scene* scene;

    const vec3 originalPos;
    vec3 newPos;
    vec3 lockedAxis{ 1, 1, 1 };
};



void ObjectTranslateCommand::execute(CommandExecutionContext& ctx)
{
    auto& scene = g::scene();
    scene.getSelectedObject() >> [&](auto obj)
    {
        auto state = ctx.pushFrame(ObjectTranslateState{ obj, scene });

        state.on(trc::Key::escape,        [](auto& state){ state.resetPlacement(); });
        state.on(trc::MouseButton::right, [](auto& state){ state.resetPlacement(); });
        state.on(trc::Key::enter,         [](auto& state, auto&& ctx){ state.applyPlacement(ctx); });
        state.on(trc::MouseButton::left,  [](auto& state, auto&& ctx){ state.applyPlacement(ctx); });

        // x and y keys are swapped because key codes use american layout
        state.on(trc::Key::x, [](auto& state){ state.lockAxes(Axis::eY | Axis::eZ); });
        state.on(trc::Key::z, [](auto& state){ state.lockAxes(Axis::eX | Axis::eZ); });
        state.on(trc::Key::y, [](auto& state){ state.lockAxes(Axis::eX | Axis::eY); });
        state.on({ trc::Key::x, trc::KeyModFlagBits::shift }, [](auto& state){ state.lockAxes(Axis::eX); });
        state.on({ trc::Key::z, trc::KeyModFlagBits::shift }, [](auto& state){ state.lockAxes(Axis::eY); });
        state.on({ trc::Key::y, trc::KeyModFlagBits::shift }, [](auto& state){ state.lockAxes(Axis::eZ); });

        state.onCursorMove([](auto& state, auto&& cursor){ state.onMouseMove(cursor); });
    };
}
