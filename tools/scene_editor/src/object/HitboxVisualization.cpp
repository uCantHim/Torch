#include "HitboxVisualization.h"

#include <trc/Torch.h>
#include <trc/TorchResources.h>
#include <trc/core/PipelineLayoutBuilder.h>
#include <trc/core/PipelineBuilder.h>

#include "DefaultAssets.h"

auto getHitboxPipeline() -> trc::Pipeline::ID
{
    static auto baseID = trc::getPipeline({});
    static auto layout = trc::PipelineRegistry<trc::TorchRenderConfig>::getPipelineLayout(baseID);

    static auto pipeline = trc::GraphicsPipelineBuilder(
            trc::PipelineRegistry<trc::TorchRenderConfig>::cloneGraphicsPipeline(baseID)
        )
        .setPolygonMode(vk::PolygonMode::eLine)
        .registerPipeline<trc::TorchRenderConfig>(
            layout,
            trc::RenderPassName{ trc::TorchRenderConfig::OPAQUE_G_BUFFER_PASS }
        );

    return pipeline;
}

auto makeHitboxDrawable(trc::Scene& scene, const Sphere& sphere) -> trc::Drawable
{
    trc::Drawable result(
        { g::geos().sphere, g::mats().undefined, false, false },
        getHitboxPipeline(),
        scene
    );
    result.setScale(sphere.radius);

    return result;
}

auto makeHitboxDrawable(trc::Scene& scene, const Capsule& caps) -> trc::Drawable
{
    trc::Drawable result(
        { g::geos().sphere, g::mats().undefined, false, false },
        getHitboxPipeline(),
        scene
    );
    result.setScale(caps.radius, caps.height, caps.radius);

    return result;
}



HitboxVisualization::HitboxVisualization(trc::Scene& scene)
    :
    scene(&scene)
{
}

void HitboxVisualization::removeFromScene()
{
    sphereDrawable.removeFromScene();
    capsuleDrawable.removeFromScene();
}

void HitboxVisualization::enableSphere(const Sphere& sphere)
{
    sphereDrawable = makeHitboxDrawable(*scene, sphere);
    attach(sphereDrawable);
    showSphere = true;
}

void HitboxVisualization::disableSphere()
{
    sphereDrawable.removeFromScene();
    showSphere = false;
}

bool HitboxVisualization::isSphereEnabled() const
{
    return showSphere;
}

void HitboxVisualization::enableCapsule(const Capsule& capsule)
{
    capsuleDrawable = makeHitboxDrawable(*scene, capsule);
    attach(capsuleDrawable);
    showCapsule = true;
}

void HitboxVisualization::disableCapsule()
{
    capsuleDrawable.removeFromScene();
    showCapsule = false;
}

bool HitboxVisualization::isCapsuleEnabled() const
{
    return showCapsule;
}
