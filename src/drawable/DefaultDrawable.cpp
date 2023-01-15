#include "trc/drawable/DefaultDrawable.h"

#include "trc/TorchRenderStages.h"
#include "trc/GBufferPass.h"
#include "trc/RenderPassShadow.h"
#include "trc/DrawablePipelines.h"

#include "trc/material/MaterialRuntime.h"
#include "trc/material/VertexShader.h" // For the DrawablePushConstIndex enum



namespace trc
{

void drawShadow(
    const drawcomp::RasterComponent& data,
    const DrawEnvironment& env,
    vk::CommandBuffer cmdBuf)
{
    auto currentRenderPass = dynamic_cast<RenderPassShadow*>(env.currentRenderPass);
    assert(currentRenderPass != nullptr);

    // Bind buffers and push constants
    data.geo.bindVertices(cmdBuf, 0);

    auto layout = *env.currentPipeline->getLayout();
    cmdBuf.pushConstants<mat4>(
        layout, vk::ShaderStageFlagBits::eVertex,
        0, data.modelMatrixId.get()
    );
    cmdBuf.pushConstants<ui32>(
        layout, vk::ShaderStageFlagBits::eVertex,
        sizeof(mat4), currentRenderPass->getShadowMatrixIndex()
    );
    if (data.geo.hasRig())
    {
        cmdBuf.pushConstants<AnimationDeviceData>(
            layout, vk::ShaderStageFlagBits::eVertex, sizeof(mat4) + sizeof(ui32),
            data.anim != AnimationEngine::ID::NONE ? data.anim.get() : AnimationDeviceData{}
        );
    }

    // Draw
    cmdBuf.drawIndexed(data.geo.getIndexCount(), 1, 0, 0, 0);
}

auto getDrawablePipelineFlags(DrawablePipelineInfo info) -> pipelines::DrawablePipelineTypeFlags
{
    pipelines::DrawablePipelineTypeFlags flags;
    assert(flags.get<pipelines::AnimationTypeFlagBits>() == pipelines::AnimationTypeFlagBits::none);
    assert(flags.get<pipelines::PipelineShadingTypeFlagBits>() == pipelines::PipelineShadingTypeFlagBits::opaque);

    if (info.transparent) {
        flags |= pipelines::PipelineShadingTypeFlagBits::transparent;
    }
    if (info.animated) {
        flags |= pipelines::AnimationTypeFlagBits::boneAnim;
    }

    return flags;
}

} // namespace trc



auto trc::determineDrawablePipeline(DrawablePipelineInfo info) -> Pipeline::ID
{
    return getDrawablePipeline(getDrawablePipelineFlags(info));
}

auto trc::determineDrawablePipeline(const DrawableCreateInfo& info) -> Pipeline::ID
{
    return determineDrawablePipeline({
        info.geo.getDeviceDataHandle().hasRig(),
        info.mat.getDeviceDataHandle().isTransparent()
    });
}

auto trc::makeDefaultDrawableRasterization(const DrawableCreateInfo& info)
    -> RasterComponentCreateInfo
{
    return makeDefaultDrawableRasterization(
        info,
        info.mat.getDeviceDataHandle().getRuntime({
            .animated=info.geo.getDeviceDataHandle().hasRig()
        }).getPipeline()
    );
}

auto trc::makeDefaultDrawableRasterization(const DrawableCreateInfo& info, Pipeline::ID pipeline)
    -> RasterComponentCreateInfo
{
    using FuncType = std::function<void(const drawcomp::RasterComponent&,
                                        const DrawEnvironment&,
                                        vk::CommandBuffer)>;

    GeometryHandle geo = info.geo.getDeviceDataHandle();
    MaterialHandle mat = info.mat.getDeviceDataHandle();
    RasterComponentCreateInfo result{
        .drawData={ .geo=geo, .mat=mat, .modelMatrixId={}, .anim={} },
        .drawFunctions={},
    };

    const bool transparent = mat.isTransparent();

    FuncType gbufferDraw = [](const drawcomp::RasterComponent& data,
                              const DrawEnvironment& env,
                              vk::CommandBuffer cmdBuf)
    {
        const bool animated = data.anim != AnimationEngine::ID::NONE;

        auto layout = *env.currentPipeline->getLayout();
        auto material = data.mat.getRuntime({ animated });
        material.pushConstants(cmdBuf, layout, DrawablePushConstIndex::eModelMatrix,
                               data.modelMatrixId.get());
        if (animated)
        {
            material.pushConstants(
                cmdBuf, layout, DrawablePushConstIndex::eAnimationData,
                data.anim.get()
            );
        }

        data.geo.bindVertices(cmdBuf, 0);
        cmdBuf.drawIndexed(data.geo.getIndexCount(), 1, 0, 0, 0);
    };

    result.drawFunctions.push_back({
        gBufferRenderStage,
        transparent ? GBufferPass::SubPasses::transparency
                         : GBufferPass::SubPasses::gBuffer,
        pipeline,
        std::move(gbufferDraw)
    });
    if (info.drawShadow)
    {
        result.drawFunctions.push_back({
            shadowRenderStage, SubPass::ID(0),
            getDrawablePipeline(
                getDrawablePipelineFlags({ geo.hasRig(), transparent })
                | pipelines::PipelineShadingTypeFlagBits::shadow
            ),
            drawShadow
        });
    }

    return result;
}
