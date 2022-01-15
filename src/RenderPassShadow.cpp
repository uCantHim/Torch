#include "RenderPassShadow.h"

#include "core/Window.h"



trc::RenderPassShadow::RenderPassShadow(
    const Window& window,
    const ShadowPassCreateInfo& info)
    :
    RenderPass(
        [&]()
        {
            std::vector<vk::AttachmentDescription> attachments{
                vk::AttachmentDescription(
                    {},
                    vk::Format::eD24UnormS8Uint,
                    vk::SampleCountFlagBits::e1,
                    vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
                    vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
                    vk::ImageLayout::eUndefined,
                    vk::ImageLayout::eShaderReadOnlyOptimal
                )
            };

            vk::AttachmentReference depthRef(0, vk::ImageLayout::eDepthStencilAttachmentOptimal);
            std::vector<vk::SubpassDescription> subpasses{
                vk::SubpassDescription(
                    {},
                    vk::PipelineBindPoint::eGraphics,
                    0, nullptr,
                    0, nullptr,
                    nullptr,
                    &depthRef
                ),
            };

            std::vector<vk::SubpassDependency> dependencies{
                vk::SubpassDependency(
                    VK_SUBPASS_EXTERNAL, 0,
                    vk::PipelineStageFlagBits::eAllCommands,
                    vk::PipelineStageFlagBits::eAllGraphics,
                    vk::AccessFlags(),
                    vk::AccessFlagBits::eDepthStencilAttachmentWrite
                    | vk::AccessFlagBits::eDepthStencilAttachmentRead,
                    vk::DependencyFlagBits::eByRegion
                ),
            };

            return window.getDevice()->createRenderPassUnique(
                vk::RenderPassCreateInfo({}, attachments, subpasses, dependencies)
            );
        }(),
        1
    ),
    resolution(info.resolution),
    shadowMatrixIndex(info.shadowIndex),
    depthImages(window.getSwapchain(), [&](ui32) {
        return vkb::Image(
            window.getDevice(),
            vk::ImageCreateInfo(
                {},
                vk::ImageType::e2D,
                vk::Format::eD24UnormS8Uint,
                vk::Extent3D(resolution.x, resolution.y, 1),
                1, 1,
                vk::SampleCountFlagBits::e1,
                vk::ImageTiling::eOptimal,
                vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled
            )
        );
    }),
    framebuffers(window.getSwapchain(), [&](ui32 i) {
        std::vector<vk::UniqueImageView> views;
        views.push_back(depthImages.getAt(i).createView(vk::ImageAspectFlagBits::eDepth));

        return Framebuffer(window.getDevice(), *renderPass, resolution, { std::move(views) });
    })
{
}

void trc::RenderPassShadow::begin(vk::CommandBuffer cmdBuf, vk::SubpassContents subpassContents)
{
    vk::ClearValue clearValue{ vk::ClearDepthStencilValue(1.0f, 0) };
    cmdBuf.beginRenderPass(
        vk::RenderPassBeginInfo(
            *renderPass,
            **framebuffers,
            vk::Rect2D({ 0, 0 }, { resolution.x, resolution.y }),
            clearValue
        ),
        subpassContents
    );

    // Set viewport and scissor
    cmdBuf.setViewport(0,
        vk::Viewport{ 0.0f, 0.0f, float(resolution.x), float(resolution.y), 0.0f, 1.0f }
    );
    cmdBuf.setScissor(0, vk::Rect2D{ { 0, 0 }, { resolution.x, resolution.y } });
}

void trc::RenderPassShadow::end(vk::CommandBuffer cmdBuf)
{
    cmdBuf.endRenderPass();
}

auto trc::RenderPassShadow::getResolution() const noexcept -> uvec2
{
    return resolution;
}

auto trc::RenderPassShadow::getShadowImage(ui32 imageIndex) const -> const vkb::Image&
{
    return depthImages.getAt(imageIndex);
}

auto trc::RenderPassShadow::getShadowImageView(ui32 imageIndex) const -> vk::ImageView
{
    return framebuffers.getAt(imageIndex).getAttachmentView(0);
}

auto trc::RenderPassShadow::getShadowMatrixIndex() const noexcept -> ui32
{
    return shadowMatrixIndex;
}
