#include "PipelineDefinitions.h"

#include <filesystem>
namespace fs = std::filesystem;

#include "core/PipelineBuilder.h"
#include "Vertex.h"
#include "AssetRegistry.h"
#include "drawable/DrawableInstanced.h"

#include "PipelineRegistry.h"
#include "DeferredRenderConfig.h"



namespace trc
{

auto makeInstancedDrawableDeferredPipeline(
    const Instance& instance,
    const DeferredRenderConfig& config) -> Pipeline
{
    // Layout
    auto layout = makePipelineLayout(
        instance.getDevice(),
        std::vector<vk::DescriptorSetLayout> {
            config.getGlobalDataDescriptorProvider().getDescriptorSetLayout(),
            config.getAssets().getDescriptorSetProvider().getDescriptorSetLayout(),
            config.getSceneDescriptorProvider().getDescriptorSetLayout(),
            config.getDeferredPassDescriptorProvider().getDescriptorSetLayout(),
        },
        std::vector<vk::PushConstantRange>{
            vk::PushConstantRange(
                vk::ShaderStageFlagBits::eFragment,
                84,
                sizeof(ui32)  // Picking ID
            ),
        }
    );

    // Pipeline
    vkb::ShaderProgram program(instance.getDevice(),
                               SHADER_DIR / "drawable/instanced.vert.spv",
                               SHADER_DIR / "drawable/deferred.frag.spv");

    vk::UniquePipeline pipeline = GraphicsPipelineBuilder::create()
        .setProgram(program)
        // Vertex attributes
        .addVertexInputBinding(
            vk::VertexInputBindingDescription(0, sizeof(Vertex), vk::VertexInputRate::eVertex),
            makeVertexAttributeDescriptions()
        )
        // Per-instance attributes
        .addVertexInputBinding(
            vk::VertexInputBindingDescription(
                1, sizeof(DrawableInstanced::InstanceDescription), vk::VertexInputRate::eInstance
            ),
            {
                // Model matrix
                vk::VertexInputAttributeDescription(6, 1, vk::Format::eR32G32B32A32Sfloat, 0),
                vk::VertexInputAttributeDescription(7, 1, vk::Format::eR32G32B32A32Sfloat, 16),
                vk::VertexInputAttributeDescription(8, 1, vk::Format::eR32G32B32A32Sfloat, 32),
                vk::VertexInputAttributeDescription(9, 1, vk::Format::eR32G32B32A32Sfloat, 48),
                // Material index
                vk::VertexInputAttributeDescription(10, 1, vk::Format::eR32Uint, 64),
            }
        )
        .addViewport(vk::Viewport(0, 0, 1, 1, 0.0f, 1.0f))
        .addScissorRect(vk::Rect2D({ 0, 0 }, { 1, 1 }))
        .disableBlendAttachments(3)
        .addDynamicState(vk::DynamicState::eViewport)
        .addDynamicState(vk::DynamicState::eScissor)
        .build(
            *instance.getDevice(),
            *layout,
            *config.getDeferredRenderPass(), RenderPassDeferred::SubPasses::gBuffer
        );

    Pipeline p{ std::move(layout), std::move(pipeline), vk::PipelineBindPoint::eGraphics };

    p.addStaticDescriptorSet(0, config.getGlobalDataDescriptorProvider());
    p.addStaticDescriptorSet(1, config.getAssets().getDescriptorSetProvider());
    p.addStaticDescriptorSet(2, config.getSceneDescriptorProvider());
    p.addStaticDescriptorSet(3, config.getDeferredPassDescriptorProvider());

    p.addDefaultPushConstantValue(84, NO_PICKABLE, vk::ShaderStageFlagBits::eFragment);

    return p;
}

auto makeInstancedDrawableShadowPipeline(
    const Instance& instance,
    const DeferredRenderConfig& config) -> Pipeline
{
    // Layout
    auto layout = makePipelineLayout(
        instance.getDevice(),
        std::vector<vk::DescriptorSetLayout>
        {
            config.getShadowDescriptorProvider().getDescriptorSetLayout(),
        },
        std::vector<vk::PushConstantRange>
        {
            vk::PushConstantRange(
                vk::ShaderStageFlagBits::eVertex,
                0,
                sizeof(ui32)  // light index
            )
        }
    );

    // Pipeline
    vkb::ShaderProgram program(instance.getDevice(),
                               SHADER_DIR / "drawable/shadow_instanced.vert.spv",
                               SHADER_DIR / "drawable/shadow.frag.spv");

    vk::UniquePipeline pipeline = GraphicsPipelineBuilder::create()
        .setProgram(program)
        // Default vertex attributes
        .addVertexInputBinding(
            vk::VertexInputBindingDescription(0, sizeof(Vertex), vk::VertexInputRate::eVertex),
            makeVertexAttributeDescriptions()
        )
        // Per-instance attributes
        .addVertexInputBinding(
            vk::VertexInputBindingDescription(
                1, sizeof(DrawableInstanced::InstanceDescription), vk::VertexInputRate::eInstance
            ),
            {
                // Model matrix
                vk::VertexInputAttributeDescription(6, 1, vk::Format::eR32G32B32A32Sfloat, 0),
                vk::VertexInputAttributeDescription(7, 1, vk::Format::eR32G32B32A32Sfloat, 16),
                vk::VertexInputAttributeDescription(8, 1, vk::Format::eR32G32B32A32Sfloat, 32),
                vk::VertexInputAttributeDescription(9, 1, vk::Format::eR32G32B32A32Sfloat, 48),
            }
        )
        .addViewport(vk::Viewport(0, 0, 1, 1, 0.0f, 1.0f))  // Dynamic state
        .addScissorRect(vk::Rect2D({ 0, 0 }, { 1, 1 }))     // Dynamic state
        .setColorBlending({}, false, vk::LogicOp::eOr, {})
        .addDynamicState(vk::DynamicState::eViewport)
        .addDynamicState(vk::DynamicState::eScissor)
        .build(*instance.getDevice(), *layout, config.getCompatibleShadowRenderPass(), 0);

    Pipeline p{ std::move(layout), std::move(pipeline), vk::PipelineBindPoint::eGraphics };
    p.addStaticDescriptorSet(0, config.getShadowDescriptorProvider());

    return p;
}

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
