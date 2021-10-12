#include "PipelineDefinitions.h"

#include <filesystem>
namespace fs = std::filesystem;

#include "core/PipelineBuilder.h"
#include "Vertex.h"
#include "AssetRegistry.h"

#include "PipelineRegistry.h"
#include "DeferredRenderConfig.h"



namespace trc
{

auto makeFinalLightingPipeline(const Instance& instance,
                               const DeferredRenderConfig& config) -> Pipeline;

PIPELINE_GETTER_FUNC(getFinalLightingPipeline, makeFinalLightingPipeline, DeferredRenderConfig);



auto makeFinalLightingPipeline(
    const Instance& instance,
    const DeferredRenderConfig& config) -> Pipeline
{
    // Layout
    auto layout = makePipelineLayout(
        instance.getDevice(),
        std::vector<vk::DescriptorSetLayout>
        {
            config.getGlobalDataDescriptorProvider().getDescriptorSetLayout(),
            config.getAssets().getDescriptorSetProvider().getDescriptorSetLayout(),
            config.getDeferredPassDescriptorProvider().getDescriptorSetLayout(),
            config.getSceneDescriptorProvider().getDescriptorSetLayout(),
            config.getShadowDescriptorProvider().getDescriptorSetLayout(),
        },
        std::vector<vk::PushConstantRange>{}
    );

    // Pipeline
    vkb::ShaderProgram program(instance.getDevice(),
                               SHADER_DIR / "final_lighting.vert.spv",
                               SHADER_DIR / "final_lighting.frag.spv");

    vk::UniquePipeline pipeline = GraphicsPipelineBuilder::create()
        .setProgram(program)
        .addVertexInputBinding(
            vk::VertexInputBindingDescription(0, sizeof(vec3), vk::VertexInputRate::eVertex),
            {
                vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32B32Sfloat, 0),
            }
        )
        .addViewport(vk::Viewport(0, 0, 1, 1, 0.0f, 1.0f))
        .addScissorRect(vk::Rect2D({ 0, 0 }, { 1, 1 }))
        .disableBlendAttachments(1)
        .addDynamicState(vk::DynamicState::eViewport)
        .addDynamicState(vk::DynamicState::eScissor)
        .build(
            *instance.getDevice(),
            *layout,
            *config.getDeferredRenderPass(), RenderPassDeferred::SubPasses::lighting
        );

    Pipeline p{ std::move(layout), std::move(pipeline), vk::PipelineBindPoint::eGraphics };
    p.addStaticDescriptorSet(0, config.getGlobalDataDescriptorProvider());
    p.addStaticDescriptorSet(1, config.getAssets().getDescriptorSetProvider());
    p.addStaticDescriptorSet(2, config.getDeferredPassDescriptorProvider());
    p.addStaticDescriptorSet(3, config.getSceneDescriptorProvider());
    p.addStaticDescriptorSet(4, config.getShadowDescriptorProvider());

    return p;
}

} // namespace trc
