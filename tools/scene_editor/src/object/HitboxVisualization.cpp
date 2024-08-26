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
    capsuleDrawable.reset();
}

void HitboxVisualization::enableSphere(const Sphere& sphere)
{
    sphereDrawable = scene->makeDrawable({ g::geos().sphere, g::mats().objectHitbox });
    sphereDrawable.value()->setScale(sphere.radius);
    attach(**sphereDrawable);
}

void HitboxVisualization::disableSphere()
{
    sphereDrawable.reset();
}

bool HitboxVisualization::isSphereEnabled() const
{
    return sphereDrawable.has_value();
}

void HitboxVisualization::enableCapsule(const Capsule& capsule)
{
    capsuleDrawable = scene->makeDrawable({ g::geos().sphere, g::mats().objectHitbox });
    capsuleDrawable.value()->setScale(capsule.radius, capsule.height, capsule.radius);
    attach(**capsuleDrawable);
}

void HitboxVisualization::disableCapsule()
{
    capsuleDrawable.reset();
}

bool HitboxVisualization::isCapsuleEnabled() const
{
    return capsuleDrawable.has_value();
}
