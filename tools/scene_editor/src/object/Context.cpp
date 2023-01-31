#include "Context.h"

#include <functional>
#include <vector>

#include "Scene.h"
#include "Hitbox.h"
#include "HitboxVisualization.h"
#include "gui/ImguiUtil.h"
#include "gui/ContextMenu.h"



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
    {
        auto& vis = scene.add<HitboxVisualization>(obj, scene.getDrawableScene());
        scene.get<trc::Drawable>(obj).attach(vis);
    }

    void operator()()
    {
        auto& vis = scene->get<HitboxVisualization>(obj);

        if (!ig::CollapsingHeader("Hitbox ")) return;
        ig::TreePush("##context_hitbox_dialog");

        ig::Text("Sphere");
        ig::TreePush("##context_hitbox_sphere_data");
            ig::Text("Radius: %.2f", hitbox.getSphere().radius);
            vec3 o = hitbox.getSphere().position;
            ig::Text("Offset: [%.2f, %.2f, %.2f]", o.x, o.y, o.z);
        ig::TreePop();
        if (bool showSphere = vis.isSphereEnabled();
            ig::Checkbox("Show spherical hitbox", &showSphere))
        {
            if (showSphere) {
                vis.enableSphere(hitbox.getSphere());
            }
            else {
                vis.disableSphere();
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
        if (bool showCapsule = vis.isCapsuleEnabled();
            ig::Checkbox("Show capsule hitbox", &showCapsule))
        {
            if (showCapsule) {
                vis.enableCapsule(hitbox.getCapsule());
            }
            else {
                vis.disableCapsule();
            }
        }

        ig::TreePop();
    }

private:
    Scene* scene;
    SceneObject obj;
    Hitbox hitbox;
};

class AnimationDialog
{
public:
    AnimationDialog(SceneObject obj, Scene& scene)
        :
        obj(obj),
        scene(&scene),
        rigId(scene.get<trc::Drawable>(obj).getGeometry().get().getRig())
    {
    }

    void operator()()
    {
        if (!ig::CollapsingHeader("Animation ")) return;
        ig::TreePush("##context_animation_dialog");

        if (!rigId)
        {
            ig::Text("No rig attached");
            ig::TreePop();
            return;
        }

        auto rig = rigId.getDeviceDataHandle();
        ig::Text("%i animations", rig.getAnimationCount());
        for (ui32 i = 0; i < rig.getAnimationCount(); i++)
        {
            ig::PushID(i);
            if (ig::Button("Play"))
            {
                scene->get<trc::Drawable>(obj).getAnimationEngine().playAnimation(i);
            }
            ig::PopID();
            ig::SameLine(0.0f, 50.0f);
            auto anim = rig.getAnimation(i);
            ig::Text("\"%s\"", scene->getAssets().getMetaData(anim).name.c_str());
        }

        ig::TreePop();
    }

private:
    const SceneObject obj;
    Scene* scene;
    const trc::RigID rigId;
};

auto makeContext(Scene& scene, SceneObject obj) -> std::function<void()>
{
    ComposedFunction func;

    scene.getM<Hitbox>(obj) >> [&](Hitbox hitbox) {
        func.add(HitboxDialog(hitbox, scene, obj));
    };
    if (scene.get<trc::Drawable>(obj).isAnimated()) {
        func.add(AnimationDialog(obj, scene));
    };

    func.add([&scene, obj]{
        ig::Spacing();
        if (ig::Button("Delete"))
        {
            scene.deleteObject(obj);
            gui::ContextMenu::close();
        }
    });

    return func;
}

void drawObjectContextMenu(Scene& scene, SceneObject obj)
{
    std::invoke(makeContext(scene, obj));
}
