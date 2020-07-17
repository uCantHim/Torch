#include "RenderPassDefinitions.h"

#include "PipelineBuilder.h"
#include "Vertex.h"
#include "AssetRegistry.h"



void trc::internal::makeRenderPasses()
{
    makeMainRenderPass();
}

void trc::internal::makePipelines(std::pair<vk::DescriptorSetLayout, vk::DescriptorSet> cameraSet)
{
    makeDrawableDeferredPipeline(cameraSet);
}

void trc::internal::makeMainRenderPass()
{
    std::vector<vk::AttachmentReference> attachmentRefs = {
        { 0, vk::ImageLayout::eColorAttachmentOptimal },
        { 1, vk::ImageLayout::eDepthStencilAttachmentOptimal },
    };

    std::vector<vk::AttachmentDescription> attachments = {
        trc::makeDefaultSwapchainColorAttachment(vkb::VulkanBase::getSwapchain()),
        trc::makeDefaultDepthStencilAttachment(),
    };

    vk::SubpassDescription subpass(
        vk::SubpassDescriptionFlags(),
        vk::PipelineBindPoint::eGraphics,
        0, nullptr,
        1, &attachmentRefs[0],
        nullptr, // resolve attachments
        &attachmentRefs[1]
    );

    vk::SubpassDependency dependency(
        VK_SUBPASS_EXTERNAL, 0,
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::AccessFlags(),
        vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite,
        vk::DependencyFlags()
    );

    trc::RenderPass::emplace(
        0,
        vk::RenderPassCreateInfo(
            vk::RenderPassCreateFlags(),
            static_cast<uint32_t>(attachments.size()), attachments.data(),
            1u, &subpass,
            1u, &dependency
        ),
        std::vector<vk::ClearValue>{
            vk::ClearColorValue(std::array<float, 4>{ 1.0, 0.4, 1.0 }),
            vk::ClearDepthStencilValue(1.0f, 0),
        }
    );
}

void trc::internal::makeDrawableDeferredPipeline(
    std::pair<vk::DescriptorSetLayout, vk::DescriptorSet> cameraSet)
{
    auto& swapchain = vkb::VulkanBase::getSwapchain();
    auto extent = swapchain.getImageExtent();

    auto& layout = PipelineLayout::emplace(
        Pipelines::eDrawableDeferred,
        std::vector<vk::DescriptorSetLayout> {
            cameraSet.first,
            AssetRegistry::getDescriptorSetLayout(),
        },
        std::vector<vk::PushConstantRange> {
            vk::PushConstantRange(
                vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
                0, sizeof(mat4) + sizeof(ui32)
            )
        }
    );

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
        .addViewport(vk::Viewport(0, 0, extent.width, extent.height, 0.0f, 1.0f))
        .addScissorRect(vk::Rect2D({ 0, 0 }, extent))
        .addColorBlendAttachment(DEFAULT_COLOR_BLEND_ATTACHMENT_DISABLED)
        .setColorBlending({}, false, vk::LogicOp::eOr, {})
        .build(
            *vkb::VulkanBase::getDevice(),
            *layout,
            *RenderPass::at(0), RenderPasses::eDeferredPass
        );

    auto& p = GraphicsPipeline::emplace(Pipelines::eDrawableDeferred, *layout, std::move(pipeline));
    p.addStaticDescriptorSet(0, cameraSet.second);
    p.addStaticDescriptorSet(1, AssetRegistry::getDescriptorSet());
}
