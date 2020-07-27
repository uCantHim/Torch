#include "PipelineDefinitions.h"

#include "PipelineBuilder.h"
#include "Vertex.h"
#include "AssetRegistry.h"
#include "DrawableInstanced.h"



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
            Animation::getDescriptorProvider().getDescriptorSetLayout(),
        },
        std::vector<vk::PushConstantRange> {
            vk::PushConstantRange(
                vk::ShaderStageFlagBits::eVertex,
                0,
                // General drawable data
                sizeof(mat4)    // model matrix
                + sizeof(ui32)  // material index
                // Animation related data
                + sizeof(ui32)  // current animation index (UINT32_MAX if no animation)
                + sizeof(uvec2) // active keyframes
                + sizeof(float) // keyframe weight
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
            makeVertexAttributeDescriptions()
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
            *renderPass, DeferredSubPasses::eGBufferPass
        );

    auto& p = GraphicsPipeline::emplace(Pipelines::eDrawableDeferred, *layout, std::move(pipeline));
    p.addStaticDescriptorSet(0, cameraDescriptorSet);
    p.addStaticDescriptorSet(1, AssetRegistry::getDescriptorSetProvider());
    p.addStaticDescriptorSet(2, Animation::getDescriptorProvider());
}

void trc::internal::makeInstancedDrawableDeferredPipeline(
    RenderPass& renderPass,
    const DescriptorProviderInterface& cameraDescriptorSet)
{
    auto& swapchain = vkb::VulkanBase::getSwapchain();
    auto extent = swapchain.getImageExtent();

    // Layout
    auto& layout = PipelineLayout::emplace(
        Pipelines::eDrawableInstancedDeferred,
        std::vector<vk::DescriptorSetLayout> {
            cameraDescriptorSet.getDescriptorSetLayout(),
            AssetRegistry::getDescriptorSetProvider().getDescriptorSetLayout(),
        },
        std::vector<vk::PushConstantRange>{}
    );

    // Pipeline
    vkb::ShaderProgram program("shaders/drawable/instanced.vert.spv",
                               "shaders/drawable/deferred.frag.spv");

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
            *renderPass, DeferredSubPasses::eGBufferPass
        );

    auto& p = GraphicsPipeline::emplace(
        Pipelines::eDrawableInstancedDeferred,
        *layout, std::move(pipeline));
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
        Pipelines::eFinalLighting,
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
            *renderPass, DeferredSubPasses::eLightingPass
        );

    auto& p = GraphicsPipeline::emplace(Pipelines::eFinalLighting, *layout, std::move(pipeline));
    p.addStaticDescriptorSet(0, generalDescriptorSet);
    p.addStaticDescriptorSet(1, AssetRegistry::getDescriptorSetProvider());
    p.addStaticDescriptorSet(2, gBufferInputSet);
}
