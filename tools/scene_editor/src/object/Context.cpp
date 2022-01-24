#include "Context.h"

#include <vector>

#include "Scene.h"
#include "Hitbox.h"
#include "HitboxVisualization.h"
#include "gui/ImguiUtil.h"



struct ComposedFunction
{
    void operator()()
    {
        for (auto& f : funcs) f();
    }

    void add(std::function<void()> func)
    {
        funcs.emplace_back(std::move(func));
    }

private:
    std::vector<std::function<void()>> funcs;
};

class HitboxDialog
{
public:
    HitboxDialog(Hitbox hitbox, Scene& scene, SceneObject obj)
        : scene(&scene), obj(obj), hitbox(hitbox)
    {}

    void operator()()
    {
        if (!ig::CollapsingHeader("Hitbox")) return;
        ig::TreePush();

        ig::Text("Sphere");
        ig::TreePush("##context_hitbox_sphere_data");
            ig::Text("Radius: %.2f", hitbox.getSphere().radius);
            vec3 o = hitbox.getSphere().position;
            ig::Text("Offset: [%.2f, %.2f, %.2f]", o.x, o.y, o.z);
        ig::TreePop();
        if (ig::Checkbox("Show spherical hitbox", &showSphere))
        {
            if (showSphere) {
                sphereReg = makeHitboxDrawable(scene->getDrawableScene(), hitbox.getSphere());
            }
            else {
                scene->getDrawableScene().unregisterDrawFunction(sphereReg);
            }
        }
        ig::Separator();

        ig::Text("Capsule");
        ig::TreePush("##context_hitbox_capsule_data");
            ig::Text("Height: %.2f", hitbox.getCapsule().height);
            ig::Text("Radius: %.2f", hitbox.getCapsule().radius);
            vec3 p = hitbox.getCapsule().position;
            ig::Text("Offset: [%.2f, %.2f, %.2f]", p.x, p.y, p.z);
        ig::TreePop();
        if (ig::Checkbox("Show capsule hitbox", &showCapsule))
        {
            if (showCapsule) {
                capsuleReg = makeHitboxDrawable(scene->getDrawableScene(), hitbox.getCapsule());
            }
            else {
                scene->getDrawableScene().unregisterDrawFunction(capsuleReg);
            }
        }

        ig::TreePop();
    }

private:
    Scene* scene;
    SceneObject obj;
    Hitbox hitbox;

    bool showSphere{ false };
    bool showCapsule{ false };
    trc::SceneBase::RegistrationID sphereReg;
    trc::SceneBase::RegistrationID capsuleReg;
};

auto makeContext(Scene& scene, SceneObject obj) -> std::function<void()>
{
    ComposedFunction func;

    scene.getM<Hitbox>(obj) >> [&](Hitbox hitbox) {
        func.add(HitboxDialog(hitbox, scene, obj));
    };

    return func;
}
