#include "PipelineDefinitions.h"

#include "PipelineBuilder.h"
#include "Vertex.h"
#include "AssetRegistry.h"



void trc::internal::makeDrawableDeferredPipeline(
    RenderPass& renderPass,
    const DescriptorProviderInterface& cameraDescriptorSet)
{
    auto& swapchain = vkb::VulkanBase::getSwapchain();
    auto extent = swapchain.getImageExtent();

    // Layout
    auto& layout = PipelineLayout::emplace(
        Pipelines::eDrawableDeferred,
        std::vector<vk::DescriptorSetLayout> {
            cameraDescriptorSet.getDescriptorSetLayout(),
            AssetRegistry::getDescriptorSetProvider().getDescriptorSetLayout(),
        },
        std::vector<vk::PushConstantRange> {
            vk::PushConstantRange(
                vk::ShaderStageFlagBits::eVertex,
                0, sizeof(mat4) + sizeof(ui32)
            ),
        }
    );

    // Pipeline
    vkb::ShaderProgram program("shaders/drawable/deferred.vert.spv",
                               "shaders/drawable/deferred.frag.spv");

    vk::UniquePipeline pipeline = GraphicsPipelineBuilder::create()
        .setProgram(program)
        .addVertexInputBinding(
            vk::VertexInputBindingDescription(0, sizeof(Vertex), vk::VertexInputRate::eVertex),
            {
                vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32B32Sfloat, 0),
                vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32Sfloat, 12),
                vk::VertexInputAttributeDescription(2, 0, vk::Format::eR32G32Sfloat, 24),
                vk::VertexInputAttributeDescription(3, 0, vk::Format::eR32G32B32Sfloat, 32),
            }
        )
        .setFrontFace(vk::FrontFace::eClockwise)
        .addViewport(vk::Viewport(0, 0, extent.width, extent.height, 0.0f, 1.0f))
        .addScissorRect(vk::Rect2D({ 0, 0 }, extent))
        .addColorBlendAttachment(DEFAULT_COLOR_BLEND_ATTACHMENT_DISABLED)
        .addColorBlendAttachment(DEFAULT_COLOR_BLEND_ATTACHMENT_DISABLED)
        .addColorBlendAttachment(DEFAULT_COLOR_BLEND_ATTACHMENT_DISABLED)
        .addColorBlendAttachment(DEFAULT_COLOR_BLEND_ATTACHMENT_DISABLED)
        .setColorBlending({}, false, vk::LogicOp::eOr, {})
        .build(
            *vkb::VulkanBase::getDevice(),
            *layout,
            *renderPass, RenderPasses::eDeferredPass
        );

    auto& p = GraphicsPipeline::emplace(Pipelines::eDrawableDeferred, *layout, std::move(pipeline));
    p.addStaticDescriptorSet(0, cameraDescriptorSet);
    p.addStaticDescriptorSet(1, AssetRegistry::getDescriptorSetProvider());
}

void trc::internal::makeFinalLightingPipeline(
    RenderPass& renderPass,
    const DescriptorProviderInterface& generalDescriptorSet,
    const DescriptorProviderInterface& gBufferInputSet)
{
    auto& swapchain = vkb::VulkanBase::getSwapchain();
    auto extent = swapchain.getImageExtent();

    // Layout
    auto& layout = PipelineLayout::emplace(
        Pipelines::eDrawableLighting,
        std::vector<vk::DescriptorSetLayout>
        {
            generalDescriptorSet.getDescriptorSetLayout(),
            AssetRegistry::getDescriptorSetProvider().getDescriptorSetLayout(),
            gBufferInputSet.getDescriptorSetLayout(),
        },
        std::vector<vk::PushConstantRange>
        {
            // Camera position
            vk::PushConstantRange(vk::ShaderStageFlagBits::eFragment, 0, sizeof(vec3)),
        }
    );

    // Pipeline
    vkb::ShaderProgram program("shaders/final_lighting.vert.spv",
                               "shaders/final_lighting.frag.spv");

    vk::UniquePipeline pipeline = GraphicsPipelineBuilder::create()
        .setProgram(program)
        .addVertexInputBinding(
            vk::VertexInputBindingDescription(0, sizeof(vec3), vk::VertexInputRate::eVertex),
            {
                vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32B32Sfloat, 0),
            }
        )
        .addViewport(vk::Viewport(0, 0, extent.width, extent.height, 0.0f, 1.0f))
        .addScissorRect(vk::Rect2D({ 0, 0 }, extent))
        .addColorBlendAttachment(DEFAULT_COLOR_BLEND_ATTACHMENT_DISABLED)
        .setColorBlending({}, false, vk::LogicOp::eOr, {})
        .build(
            *vkb::VulkanBase::getDevice(),
            *layout,
            *renderPass, RenderPasses::eLightingPass
        );

    auto& p = GraphicsPipeline::emplace(Pipelines::eDrawableLighting, *layout, std::move(pipeline));
    p.addStaticDescriptorSet(0, generalDescriptorSet);
    p.addStaticDescriptorSet(1, AssetRegistry::getDescriptorSetProvider());
    p.addStaticDescriptorSet(2, gBufferInputSet);
}
