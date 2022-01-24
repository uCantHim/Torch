#include "HitboxVisualization.h"

#include <trc/Torch.h>
#include <trc/TorchResources.h>
#include <trc/core/PipelineLayoutBuilder.h>
#include <trc/core/PipelineBuilder.h>

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

auto makeHitboxDrawable(trc::SceneBase& scene, const Sphere& sphere)
    -> trc::MaybeUniqueRegistrationId
{
    return scene.registerDrawFunction(
        trc::gBufferRenderStage, trc::GBufferPass::SubPasses::gBuffer, getHitboxPipeline(),
        [](auto& env, vk::CommandBuffer cmdBuf)
        {
        }
    );
}

auto makeHitboxDrawable(trc::SceneBase& scene, const Capsule& caps)
    -> trc::MaybeUniqueRegistrationId
{
    return scene.registerDrawFunction(
        trc::gBufferRenderStage, trc::GBufferPass::SubPasses::gBuffer, getHitboxPipeline(),
        [](auto& env, vk::CommandBuffer cmdBuf)
        {
        }
    );
}
