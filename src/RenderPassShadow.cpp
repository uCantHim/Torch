#include "RenderPassShadow.h"



trc::RenderPassShadow::RenderPassShadow(uvec2 resolution)
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
                    vk::ImageLayout::eDepthStencilAttachmentOptimal,
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

            return vkb::getDevice()->createRenderPassUnique(
                vk::RenderPassCreateInfo({}, attachments, subpasses, dependencies)
            );
        }(),
        1
    ),
    resolution(resolution),
    depthImages([&resolution](ui32) {
        return vkb::Image(
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
    framebuffers([&](ui32 imageIndex) {
        std::vector<vk::UniqueImageView> views;
        views.push_back(depthImages.getAt(imageIndex).createView(vk::ImageAspectFlagBits::eDepth));

        return Framebuffer(vkb::getDevice(), *renderPass, resolution, { std::move(views) });
    })
{
}

void trc::RenderPassShadow::begin(vk::CommandBuffer cmdBuf, vk::SubpassContents subpassContents)
{
    depthImages->changeLayout(
        cmdBuf,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eDepthStencilAttachmentOptimal,
        { vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil, 0, 1, 0, 1 }
    );

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
}

void trc::RenderPassShadow::end(vk::CommandBuffer cmdBuf)
{
    cmdBuf.endRenderPass();
}

auto trc::RenderPassShadow::getResolution() const noexcept -> uvec2
{
    return resolution;
}

auto trc::RenderPassShadow::getDepthImage(ui32 imageIndex) const -> const vkb::Image&
{
    return depthImages.getAt(imageIndex);
}

auto trc::RenderPassShadow::getDepthImageView(ui32 imageIndex) const -> vk::ImageView
{
    return framebuffers.getAt(imageIndex).getAttachmentView(0);
}

auto trc::RenderPassShadow::getShadowMatrixIndex() const noexcept -> ui32
{
    return shadowMatrixIndex;
}

void trc::RenderPassShadow::setShadowMatrixIndex(ui32 newIndex) noexcept
{
    shadowMatrixIndex = newIndex;
}
