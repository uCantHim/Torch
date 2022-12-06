#include "HitboxVisualization.h"

#include <trc/DrawablePipelines.h>
#include <trc/Torch.h>
#include <trc/TorchRenderStages.h>
#include <trc/core/PipelineBuilder.h>
#include <trc/core/PipelineLayoutBuilder.h>
#include <trc/drawable/DefaultDrawable.h>

#include "asset/DefaultAssets.h"

auto getHitboxPipeline() -> trc::Pipeline::ID
{
    static auto baseID = trc::pipelines::getDrawablePipeline(
        trc::pipelines::PipelineShadingTypeFlagBits::opaque
        | trc::pipelines::AnimationTypeFlagBits::none
    );
    static auto layout = trc::PipelineRegistry::getPipelineLayout(baseID);

    static auto pipeline = trc::GraphicsPipelineBuilder(
            trc::PipelineRegistry::cloneGraphicsPipeline(baseID)
        )
        .setPolygonMode(vk::PolygonMode::eLine)
        .registerPipeline(
            layout,
            trc::RenderPassName{ trc::TorchRenderConfig::OPAQUE_G_BUFFER_PASS }
        );

    return pipeline;
}

auto makeHitboxDrawable(trc::Scene& scene, const Sphere& sphere) -> trc::UniqueDrawableID
{
    auto drawable = scene.makeUniqueDrawable();

    trc::Node& node = scene.makeNode(drawable);
    node.setScale(sphere.radius);

    auto rasterComponent = trc::makeDefaultDrawableRasterization(
        { g::geos().sphere, g::mats().undefined, false, false },
        getHitboxPipeline()
    );
    rasterComponent.drawData.modelMatrixId = node.getGlobalTransformID();
    scene.makeRasterization(drawable, std::move(rasterComponent));

    return drawable;
}

auto makeHitboxDrawable(trc::Scene& scene, const Capsule& caps) -> trc::UniqueDrawableID
{
    auto drawable = scene.makeUniqueDrawable();

    trc::Node& node = scene.makeNode(drawable);
    node.setScale(caps.radius, caps.height, caps.radius);

    auto rasterComponent = trc::makeDefaultDrawableRasterization(
        { g::geos().sphere, g::mats().undefined, false, false },
        getHitboxPipeline()
    );
    rasterComponent.drawData.modelMatrixId = node.getGlobalTransformID();
    scene.makeRasterization(drawable, std::move(rasterComponent));

    return drawable;
}



HitboxVisualization::HitboxVisualization(trc::Scene& scene)
    :
    scene(&scene)
{
}

void HitboxVisualization::removeFromScene()
{
    scene->destroyDrawable(sphereDrawable);
    scene->destroyDrawable(capsuleDrawable);
}

void HitboxVisualization::enableSphere(const Sphere& sphere)
{
    sphereDrawable = makeHitboxDrawable(*scene, sphere);
    attach(scene->getNode(sphereDrawable));
    showSphere = true;
}

void HitboxVisualization::disableSphere()
{
    scene->destroyDrawable(sphereDrawable);
    showSphere = false;
}

bool HitboxVisualization::isSphereEnabled() const
{
    return showSphere;
}

void HitboxVisualization::enableCapsule(const Capsule& capsule)
{
    capsuleDrawable = makeHitboxDrawable(*scene, capsule);
    attach(scene->getNode(capsuleDrawable));
    showCapsule = true;
}

void HitboxVisualization::disableCapsule()
{
    scene->destroyDrawable(capsuleDrawable);
    showCapsule = false;
}

bool HitboxVisualization::isCapsuleEnabled() const
{
    return showCapsule;
}
