#include "trc/drawable/RasterComponent.h"

#include <cassert>

#include "trc/TorchRenderStages.h"
#include "trc/GBufferPass.h"



trc::RasterComponent::RasterComponent(const RasterComponentCreateInfo& createInfo)
{
    auto geoHandle = createInfo.geo.getDeviceDataHandle();
    auto matHandle = createInfo.mat.getDeviceDataHandle();

    isAnimated = geoHandle.hasRig() && createInfo.anim != AnimationEngine::ID::NONE;
    transparentLighting = matHandle.isTransparent();
    hasShadow = !createInfo.disableShadow;

    this->drawInfo = std::make_shared<DrawableRasterDrawInfo>(DrawableRasterDrawInfo{
        .geo=geoHandle,
        .mat=matHandle,
        .matRuntime=matHandle.getRuntime({
            .animated=this->isAnimated,
        }),
        .modelMatrixId=createInfo.modelMatrixId,
        .anim=createInfo.anim,
    });
}

void componentlib::ComponentTraits<trc::RasterComponent>::onCreate(
    trc::DrawableScene& scene,
    trc::DrawableID /*drawable*/,
    trc::RasterComponent& comp)
{
    using namespace trc;

    assert(comp.drawInfo != nullptr);
    assert(comp.drawFuncs.empty());

    const DrawablePipelineInfo pipelineInfo{
        .animated=comp.isAnimated,
        .transparent=comp.transparentLighting,
    };
    RasterSceneBase& base = scene.getRasterModule();

    // Create a storage for the draw functions with automatic lifetime
    using SubPasses = GBufferPass::SubPasses;
    comp.drawFuncs.emplace_back(
        base.registerDrawFunction(
            stages::gBuffer,
            pipelineInfo.transparent ? SubPasses::transparency : SubPasses::gBuffer,
            comp.drawInfo->matRuntime->getPipeline(),
            makeGBufferDrawFunction(comp.drawInfo)
        )
    );
    if (comp.hasShadow)
    {
        comp.drawFuncs.emplace_back(
            base.registerDrawFunction(
                stages::shadow,
                SubPass::ID(0),
                pipelineInfo.determineShadowPipeline(),
                makeShadowDrawFunction(comp.drawInfo)
            )
        );
    }
}
