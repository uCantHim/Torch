#include "trc/RasterTasks.h"

#include "trc/RasterSceneModule.h"
#include "trc/ShadowPool.h"
#include "trc/TorchRenderStages.h"
#include "trc/core/Frame.h"
#include "trc/core/ResourceConfig.h"
#include "trc/core/SceneBase.h"



namespace trc
{

RenderPassDrawTask::RenderPassDrawTask(
    RenderStage::ID renderStage,
    s_ptr<RenderPass> _renderPass)
    :
    renderStage(renderStage),
    renderPass(std::move(_renderPass))
{
    if (renderPass == nullptr) {
        throw std::invalid_argument("[In RenderPassDrawTask::RenderPassDrawTask]: Render pass"
                                    " must not be nullptr!");
    }
}

void RenderPassDrawTask::record(vk::CommandBuffer cmdBuf, ViewportDrawContext& ctx)
{
    auto& scene = ctx.scene().getModule<RasterSceneModule>();

    renderPass->begin(cmdBuf, vk::SubpassContents::eInline, ctx.frame());

    // Record all commands
    const ui32 subPassCount = renderPass->getNumSubPasses();
    for (ui32 subPass = 0; subPass < subPassCount; subPass++)
    {
        for (auto pipeline : scene.iterPipelines(renderStage, SubPass::ID(subPass)))
        {
            // Bind the current pipeline
            auto& p = ctx.resources().getPipeline(pipeline);
            p.bind(cmdBuf, ctx.resources());

            // Record commands for all objects with this pipeline
            scene.invokeDrawFunctions(
                renderStage, *renderPass, SubPass::ID(subPass),
                pipeline, p,
                cmdBuf
            );
        }

        if (subPass < subPassCount - 1) {
            cmdBuf.nextSubpass(vk::SubpassContents::eInline);
        }
    }

    renderPass->end(cmdBuf);
}



ShadowMapDrawTask::ShadowMapDrawTask(RenderStage::ID stage, s_ptr<ShadowMap> shadowMap)
    :
    renderStage(stage),
    shadowMap(std::move(shadowMap))
{}

void ShadowMapDrawTask::record(vk::CommandBuffer cmdBuf, SceneUpdateContext& ctx)
{
    auto& scene = ctx.scene().getModule<RasterSceneModule>();
    auto& renderPass = shadowMap->getRenderPass();

    renderPass.begin(cmdBuf, vk::SubpassContents::eInline, ctx.frame());

    // Record all commands
    const ui32 subPassCount = renderPass.getNumSubPasses();
    for (ui32 subPass = 0; subPass < subPassCount; subPass++)
    {
        for (auto pipeline : scene.iterPipelines(renderStage, SubPass::ID(subPass)))
        {
            // Bind the current pipeline
            auto& p = ctx.resources().getPipeline(pipeline);
            p.bind(cmdBuf, ctx.resources());

            // Record commands for all objects with this pipeline
            scene.invokeDrawFunctions(
                renderStage, renderPass, SubPass::ID(subPass),
                pipeline, p,
                cmdBuf
            );
        }

        if (subPass < subPassCount - 1) {
            cmdBuf.nextSubpass(vk::SubpassContents::eInline);
        }
    }

    renderPass.end(cmdBuf);
}

} // namespace trc
