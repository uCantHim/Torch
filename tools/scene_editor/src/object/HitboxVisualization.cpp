#include "HitboxVisualization.h"

#include <trc/DrawablePipelines.h>
#include <trc/TorchRenderStages.h>
#include <trc/core/PipelineBuilder.h>
#include <trc/core/PipelineLayoutBuilder.h>
#include <trc/drawable/DefaultDrawable.h>

#include "asset/DefaultAssets.h"



HitboxVisualization::HitboxVisualization(trc::Scene& scene)
    :
    scene(&scene)
{
}

void HitboxVisualization::removeFromScene()
{
    sphereDrawable.reset();
    capsuleDrawables = {};
}

void HitboxVisualization::enableSphere(const Sphere& sphere)
{
    sphereDrawable = scene->makeDrawable({
        .geo=g::geos().sphere,
        .mat=g::mats().objectHitbox,
        .disableShadow=true,
    });
    sphereDrawable->setScale(sphere.radius).setTranslation(sphere.position);
    attach(*sphereDrawable);
}

void HitboxVisualization::disableSphere()
{
    sphereDrawable.reset();
}

bool HitboxVisualization::isSphereEnabled() const
{
    return !!sphereDrawable;
}

void HitboxVisualization::enableCapsule(const Capsule& capsule)
{
    capsuleDrawables[0] = scene->makeDrawable({
        .geo=g::geos().halfSphere,
        .mat=g::mats().objectHitbox,
        .disableShadow=true,
    });
    capsuleDrawables[1] = scene->makeDrawable({
        .geo=g::geos().openCylinder,
        .mat=g::mats().objectHitbox,
        .disableShadow=true,
    });
    capsuleDrawables[2] = scene->makeDrawable({
        .geo=g::geos().halfSphere,
        .mat=g::mats().objectHitbox,
        .disableShadow=true,
    });

    capsuleDrawables[0]->scale(capsule.radius)
                       .rotate(glm::pi<float>(), 0, 0)
                       .translate(0, -capsule.height / 2.0f, 0)
                       .translate(capsule.position);
    capsuleDrawables[1]->scale(capsule.radius, capsule.height / 2.0f, capsule.radius)
                       .translate(capsule.position);
    capsuleDrawables[2]->scale(capsule.radius)
                       .translate(0, capsule.height / 2.0f, 0)
                       .translate(capsule.position);
    attach(*capsuleDrawables[0]);
    attach(*capsuleDrawables[1]);
    attach(*capsuleDrawables[2]);
}

void HitboxVisualization::disableCapsule()
{
    capsuleDrawables = {};
}

bool HitboxVisualization::isCapsuleEnabled() const
{
    return !!capsuleDrawables[0];
}
