#include "trc/drawable/RasterComponent.h"

#include <cassert>

#include "trc/TorchRenderStages.h"
#include "trc/GBufferPass.h"



trc::RasterComponent::RasterComponent(const RasterComponentCreateInfo& createInfo)
{
    auto geoHandle = createInfo.geo.getDeviceDataHandle();
    auto matHandle = createInfo.mat.getDeviceDataHandle();

    this->drawInfo = std::make_shared<DrawableRasterDrawInfo>(DrawableRasterDrawInfo{
        .geo=geoHandle,
        .mat=matHandle,
        .matRuntime=matHandle.getRuntime({
            .animated=geoHandle.hasRig(),
        }),
        .modelMatrixId=createInfo.modelMatrixId,
        .anim=createInfo.anim,
    });
}

void componentlib::ComponentTraits<trc::RasterComponent>::onCreate(
    trc::DrawableScene& storage,
    trc::DrawableID /*drawable*/,
    trc::RasterComponent& comp)
{
    using namespace trc;

    assert(comp.drawInfo != nullptr);
    assert(comp.drawFuncs.empty());

    const DrawablePipelineInfo pipelineInfo{
        .animated=comp.drawInfo->geo.hasRig(),
        .transparent=comp.drawInfo->mat.isTransparent(),
    };
    RasterSceneBase& base = storage.getRasterModule();

    // Create a storage for the draw functions with automatic lifetime
    using SubPasses = GBufferPass::SubPasses;
    comp.drawFuncs.emplace_back(
        base.registerDrawFunction(
            gBufferRenderStage,
            pipelineInfo.transparent ? SubPasses::transparency : SubPasses::gBuffer,
            comp.drawInfo->matRuntime.getPipeline(),
            makeGBufferDrawFunction(comp.drawInfo)
        )
    );
    comp.drawFuncs.emplace_back(
        base.registerDrawFunction(
            shadowRenderStage,
            SubPass::ID(0),
            pipelineInfo.determineShadowPipeline(),
            makeShadowDrawFunction(comp.drawInfo)
        )
    );
}
