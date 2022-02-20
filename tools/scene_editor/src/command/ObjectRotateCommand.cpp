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
        scene(&scene),
        originalOrientation(scene.get<ObjectBaseNode>(obj).getRotation()),
        pivot(scene.get<ObjectBaseNode>(obj).getGlobalTransform()[3]),
        depth(scene.getCamera().calcScreenDepth(pivot)),
        originalDir(glm::normalize(scene.getMousePosAtDepth(depth) - pivot)),
        rotationAxis(glm::normalize(pivot - vec3(scene.getCamera().getViewMatrix()[3])))
    {
        assert(glm::all(glm::not_(glm::isnan(originalDir))));
        assert(glm::all(glm::not_(glm::isnan(rotationAxis))));
        assert(glm::all(glm::not_(glm::isnan(pivot))));
    }

    bool update(const float) override
    {
        const quat rotation = glm::angleAxis(getNewAngle(), rotationAxis) * originalOrientation;
        scene->get<ObjectBaseNode>(obj).setRotation(rotation);

        return terminate;
    }

    void onExit() override
    {
        const quat rotation = glm::angleAxis(finalAngle, rotationAxis) * originalOrientation;
        scene->get<ObjectBaseNode>(obj).setRotation(rotation);
    }

    void applyRotation()
    {
        finalAngle = getNewAngle();
        terminate = true;
    }

    void resetRotation()
    {
        finalAngle = 0.0f;
        terminate = true;
    }

    void lockAxes(AxisFlags flags)
    {
        assert(glm::length(toVector(flags)) > 0.0f);
        rotationAxis = glm::normalize(toVector(flags));
    }

private:
    auto getNewAngle() const -> float
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
    vec3 rotationAxis;

    bool terminate{ false };
    float finalAngle{ 0.0f };
};



void ObjectRotateCommand::execute(CommandCall& call)
{
    auto& scene = App::get().getScene();
    scene.getSelectedObject() >> [&](auto obj)
    {
        auto& state = call.setState(ObjectRotateState{ obj, scene });

        call.on(vkb::Key::escape,        [&](auto&){ state.resetRotation(); });
        call.on(vkb::MouseButton::right, [&](auto&){ state.resetRotation(); });
        call.on(vkb::Key::enter,         [&](auto&){ state.applyRotation(); });
        call.on(vkb::MouseButton::left,  [&](auto&){ state.applyRotation(); });

        // x and y keys are swapped because it seems like glfw uses the american keyboard (why?)
        auto shift = vkb::KeyModFlagBits::shift;
        call.on({ vkb::Key::x }, [&](auto&){ state.lockAxes(Axis::eY | Axis::eZ); });
        call.on({ vkb::Key::z }, [&](auto&){ state.lockAxes(Axis::eX | Axis::eZ); });
        call.on({ vkb::Key::y }, [&](auto&){ state.lockAxes(Axis::eX | Axis::eY); });
        call.on({ vkb::Key::x, shift }, [&](auto&){ state.lockAxes(Axis::eY | Axis::eZ); });
        call.on({ vkb::Key::z, shift }, [&](auto&){ state.lockAxes(Axis::eX | Axis::eZ); });
        call.on({ vkb::Key::y, shift }, [&](auto&){ state.lockAxes(Axis::eX | Axis::eY); });
    };
}
