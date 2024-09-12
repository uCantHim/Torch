#include "trc/RenderPassShadow.h"

#include <ranges>



trc::RenderPassShadow::RenderPassShadow(
    const Device& device,
    const ShadowPassCreateInfo& info,
    const DeviceMemoryAllocator& alloc)
    :
    RenderPass(makeVkRenderPass(device), 1),
    resolution(info.resolution),
    shadowMatrixIndex(info.shadowIndex),
    depthImage{
        device,
        vk::ImageCreateInfo(
            {},
            vk::ImageType::e2D,
            vk::Format::eD24UnormS8Uint,
            vk::Extent3D(resolution.x, resolution.y, 1),
            1, 1,
            vk::SampleCountFlagBits::e1,
            vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled
        ),
        alloc
    },
    depthImageView(depthImage.createView(vk::ImageAspectFlagBits::eDepth)),
    framebuffer{
        device,
        *renderPass,
        resolution,
        *depthImageView
    }
{
}

void trc::RenderPassShadow::begin(
    vk::CommandBuffer cmdBuf,
    vk::SubpassContents subpassContents,
    FrameRenderState&)
{
    vk::ClearValue clearValue{ vk::ClearDepthStencilValue(1.0f, 0) };
    cmdBuf.beginRenderPass(
        vk::RenderPassBeginInfo(
            *renderPass,
            *framebuffer,
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

auto trc::RenderPassShadow::getShadowImage() const -> const Image&
{
    return depthImage;
}

auto trc::RenderPassShadow::getShadowImageView() const -> vk::ImageView
{
    return framebuffer.getAttachmentView(0);
}

auto trc::RenderPassShadow::getShadowMatrixIndex() const noexcept -> ui32
{
    return shadowMatrixIndex;
}

auto trc::RenderPassShadow::makeVkRenderPass(const Device& device) -> vk::UniqueRenderPass
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
            vk::PipelineStageFlagBits::eLateFragmentTests,
            vk::PipelineStageFlagBits::eFragmentShader,
            vk::AccessFlagBits::eDepthStencilAttachmentWrite,
            vk::AccessFlagBits::eShaderRead,
            vk::DependencyFlagBits::eByRegion
        ),
    };

    return device->createRenderPassUnique(
        vk::RenderPassCreateInfo({}, attachments, subpasses, dependencies)
    );
}
