#include "flora/FloraPipelines.h"

#include "RenderPassDeferred.h"
#include "PipelineDefinitions.h"
#include "PipelineRegistry.h"



namespace trc
{
    auto makeGrassPipeline(
        const vkb::Device& device,
        vk::RenderPass renderPass,
        SubPass::ID subPass) -> Pipeline
    {
        auto layout = makePipelineLayout({}, {});

        auto pipeline = GraphicsPipelineBuilder::create()
            .setProgram({
                TRC_SHADER_DIR"/flora/grass.vert",
                TRC_SHADER_DIR"/flora/grass.frag"
            })
            .build({}, *layout, renderPass, subPass);

        return { std::move(layout), std::move(pipeline), vk::PipelineBindPoint::eGraphics };
    }
} // namespace trc

auto trc::getGrassPipeline() -> Pipeline::ID
{
    static Pipeline::ID id = PipelineRegistry::registerPipeline([] {
        auto renderPass = RenderPassDeferred::makeVkRenderPassInstance(vkb::getSwapchain());

        // Create pipeline for the first time
        return makeGrassPipeline(
            vkb::getDevice(),
            *renderPass,
            internal::DeferredSubPasses::eGBufferPass
        );
    });

    return id;
}
