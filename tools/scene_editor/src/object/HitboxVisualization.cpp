#include "HitboxVisualization.h"

#include <trc/Torch.h>
#include <trc/TorchResources.h>
#include <trc/core/PipelineLayoutBuilder.h>
#include <trc/core/PipelineBuilder.h>

#include "AssetManager.h"
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

auto makeHitboxDrawable(const Sphere& sphere) -> trc::Drawable
{
    trc::Drawable result(
        { g::geos().sphere, g::mats().undefined, false, false },
        getHitboxPipeline()
    );
    result.setScale(sphere.radius);

    return result;
}

auto makeHitboxDrawable(const Capsule& caps) -> trc::Drawable
{
    trc::Drawable result(
        { g::geos().sphere, g::mats().undefined, false, false },
        getHitboxPipeline()
    );
    result.setScale(caps.radius, caps.height, caps.radius);

    return result;
}



void HitboxVisualization::attachToScene(trc::SceneBase& newScene)
{
    if (scene != nullptr)
    {
        sphereDrawable.removeFromScene();
        capsuleDrawable.removeFromScene();
    }

    sphereDrawable.attachToScene(newScene);
    capsuleDrawable.attachToScene(newScene);
    scene = &newScene;
}

void HitboxVisualization::removeFromScene()
{
    if (scene != nullptr)
    {
        sphereDrawable.removeFromScene();
        capsuleDrawable.removeFromScene();
        scene = nullptr;
    }
}

void HitboxVisualization::enableSphere(const Sphere& sphere)
{
    sphereDrawable = makeHitboxDrawable(sphere);
    attach(sphereDrawable);
    if (scene != nullptr) {
        sphereDrawable.attachToScene(*scene);
    }
    showSphere = true;
}

void HitboxVisualization::disableSphere()
{
    if (scene != nullptr) {
        sphereDrawable.removeFromScene();
    }
    showSphere = false;
}

bool HitboxVisualization::isSphereEnabled() const
{
    return showSphere;
}

void HitboxVisualization::enableCapsule(const Capsule& capsule)
{
    capsuleDrawable = makeHitboxDrawable(capsule);
    attach(capsuleDrawable);
    if (scene != nullptr) {
        capsuleDrawable.attachToScene(*scene);
    }
    showCapsule = true;
}

void HitboxVisualization::disableCapsule()
{
    if (scene != nullptr) {
        capsuleDrawable.removeFromScene();
    }
    showCapsule = false;
}

bool HitboxVisualization::isCapsuleEnabled() const
{
    return showCapsule;
}
