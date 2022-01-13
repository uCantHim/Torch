#include "RenderPassDeferred.h"

#include <glm/detail/type_half.hpp>

#include "Camera.h"



trc::RenderPassDeferred::RenderPassDeferred(
    const vkb::Device& device,
    const vkb::Swapchain& swapchain,
    vkb::FrameSpecific<GBuffer>& gBuffer)
    :
    RenderPass(makeVkRenderPass(device), NUM_SUBPASSES),
    depthPixelReadBuffer(
        device,
        sizeof(float),
        vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eDeviceLocal
        | vk::MemoryPropertyFlagBits::eHostCoherent
    ),
    swapchain(swapchain),
    gBuffer(gBuffer),
    framebufferSize(gBuffer->getSize()),
    framebuffers(swapchain, [&](ui32 frameIndex)
    {
        const GBuffer& g = gBuffer.getAt(frameIndex);

        std::vector<vk::ImageView> views{
            g.getImageView(GBuffer::eNormals),
            g.getImageView(GBuffer::eAlbedo),
            g.getImageView(GBuffer::eMaterials),
            g.getImageView(GBuffer::eDepth),
        };

        return Framebuffer(device, *renderPass, g.getSize(), {}, std::move(views));
    }),
    clearValues({
        vk::ClearColorValue(std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 0.0f }),
        vk::ClearColorValue(std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 0.0f }),
        vk::ClearColorValue(std::array<ui32, 4>{ 0, 0, 0, 0 }),
        vk::ClearDepthStencilValue(1.0f, 0.0f),
    })
{
}

void trc::RenderPassDeferred::begin(vk::CommandBuffer cmdBuf, vk::SubpassContents subpassContents)
{
    gBuffer->initFrame(cmdBuf);

    // Bring depth image into depthStencil layout
    cmdBuf.pipelineBarrier(
        vk::PipelineStageFlagBits::eBottomOfPipe,
        vk::PipelineStageFlagBits::eAllGraphics,
        vk::DependencyFlagBits::eByRegion,
        {}, {},
        vk::ImageMemoryBarrier(
            {},
            vk::AccessFlagBits::eDepthStencilAttachmentRead
            | vk::AccessFlagBits::eDepthStencilAttachmentWrite,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eDepthStencilAttachmentOptimal,
            VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
            *gBuffer->getImage(GBuffer::eDepth),
            vk::ImageSubresourceRange(
                vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil,
                0, 1, 0, 1
            )
        )
    );

    cmdBuf.beginRenderPass(
        vk::RenderPassBeginInfo(
            *renderPass,
            **framebuffers,
            vk::Rect2D({ 0, 0 }, { framebufferSize.x, framebufferSize.y }),
            clearValues
        ),
        subpassContents
    );

    // Set viewport and scissor
#ifdef TRC_FLIP_Y_PROJECTION
    cmdBuf.setViewport(0,
        vk::Viewport{
            0.0f, float(framebufferSize.y),
            float(framebufferSize.x), -float(framebufferSize.y),
            0.0f, 1.0f
        }
    );
#else
    cmdBuf.setViewport(0,
        vk::Viewport{
            0.0f, 0.0f,
            float(framebufferSize.x), float(framebufferSize.y),
            0.0f, 1.0f
        }
    );
#endif

    cmdBuf.setScissor(0, vk::Rect2D{ { 0, 0 }, { framebufferSize.x, framebufferSize.y } });
}

void trc::RenderPassDeferred::end(vk::CommandBuffer cmdBuf)
{
    cmdBuf.endRenderPass();
    copyMouseDataToBuffers(cmdBuf);
}

void trc::RenderPassDeferred::setClearColor(const vec4 c)
{
    clearValues[1] = vk::ClearColorValue(std::array<float, 4>{ c.r, c.g, c.b, c.a });
}

auto trc::RenderPassDeferred::getMouseDepth() const noexcept -> float
{
    const ui32 depthValueD24S8 = depthBufMap[0];

    // Don't ask me why 16 bit here, I think it should be 24. The result is
    // correct when we use 65536 as depth 1.0 (maximum depth) though.
    constexpr float maxFloat16 = 65536.0f;  // 2^16
    return static_cast<float>(depthValueD24S8 >> 8) / maxFloat16;
}

auto trc::RenderPassDeferred::getMousePos(const Camera& camera) const noexcept -> vec3
{
    const vec2 resolution{ framebufferSize };
    const vec2 mousePos = glm::clamp([=, this]() -> vec2 {
#ifdef TRC_FLIP_Y_PROJECTION
        return { swapchain.getMousePosition().x, resolution.y - swapchain.getMousePosition().y, };
#else
        return swapchain.getMousePosition();
#endif
    }(), vec2(0.0f, 0.0f), resolution - 1.0f);
    const float depth = getMouseDepth();

    const vec4 clipSpace = vec4(mousePos / resolution * 2.0f - 1.0f, depth, 1.0);
    const vec4 viewSpace = glm::inverse(camera.getProjectionMatrix()) * clipSpace;
    const vec4 worldSpace = glm::inverse(camera.getViewMatrix()) * (viewSpace / viewSpace.w);

    return worldSpace;
}

auto trc::RenderPassDeferred::makeVkRenderPass(const vkb::Device& device)
    -> vk::UniqueRenderPass
{
    std::vector<vk::AttachmentDescription> attachments = {
        // Normals
        vk::AttachmentDescription(
            {}, vk::Format::eR16G16B16A16Sfloat, vk::SampleCountFlagBits::e1,
            vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
            vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
            vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral
        ),
        // Albedo
        vk::AttachmentDescription(
            {}, vk::Format::eR8G8B8A8Unorm, vk::SampleCountFlagBits::e1,
            vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
            vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
            vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral
        ),
        // Material indices
        vk::AttachmentDescription(
            {}, vk::Format::eR32Uint, vk::SampleCountFlagBits::e1,
            vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
            vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
            vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral
        ),
        // Depth-/Stencil buffer
        vk::AttachmentDescription(
            vk::AttachmentDescriptionFlags(),
            vk::Format::eD24UnormS8Uint,
            vk::SampleCountFlagBits::e1,
            vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, // load/store ops
            vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eDontCare, // stencil ops
            vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal
        ),
    };

    std::vector<vk::AttachmentReference> deferredOutput = {
        { 0, vk::ImageLayout::eColorAttachmentOptimal }, // Normals
        { 1, vk::ImageLayout::eColorAttachmentOptimal }, // UVs
        { 2, vk::ImageLayout::eColorAttachmentOptimal }, // Material indices
    };
    vk::AttachmentReference depthAttachment{ 3, vk::ImageLayout::eDepthStencilAttachmentOptimal };

    std::vector<vk::SubpassDescription> subpasses = {
        // Deferred diffuse subpass
        vk::SubpassDescription(
            vk::SubpassDescriptionFlags(),
            vk::PipelineBindPoint::eGraphics,
            {}, // input attachments
            deferredOutput,
            {}, // resolve attachments
            &depthAttachment
        ),
        // Deferred transparency subpass
        vk::SubpassDescription(
            vk::SubpassDescriptionFlags(),
            vk::PipelineBindPoint::eGraphics,
            0, nullptr, // input attachments
            0, nullptr, // color attachments
            nullptr, // resolve attachments
            &depthAttachment
        ),
    };

    std::vector<vk::SubpassDependency> dependencies = {
        // The second subpass uses the first's depth buffer for early fragment discard
        vk::SubpassDependency(
            0, 1,
            vk::PipelineStageFlagBits::eLateFragmentTests,
            vk::PipelineStageFlagBits::eEarlyFragmentTests,
            vk::AccessFlagBits::eDepthStencilAttachmentWrite,
            vk::AccessFlagBits::eDepthStencilAttachmentRead,
            vk::DependencyFlagBits::eByRegion
        ),
        // From transparency subpass to final lighting compute pass
        vk::SubpassDependency(
            1, VK_SUBPASS_EXTERNAL,
            vk::PipelineStageFlagBits::eFragmentShader,
            vk::PipelineStageFlagBits::eComputeShader,
            vk::AccessFlagBits::eShaderWrite,
            vk::AccessFlagBits::eShaderRead,
            vk::DependencyFlagBits::eByRegion
        )
    };

    return device->createRenderPassUnique(
        vk::RenderPassCreateInfo({}, attachments, subpasses, dependencies)
    );
}

void trc::RenderPassDeferred::copyMouseDataToBuffers(vk::CommandBuffer cmdBuf)
{
    vkb::Image& depthImage = gBuffer->getImage(GBuffer::eDepth);
    const ivec2 size{ depthImage.getSize().width, depthImage.getSize().height };
    const ivec2 mousePos = glm::clamp(ivec2(swapchain.getMousePosition()), ivec2(0), size - 1);

    cmdBuf.pipelineBarrier(
        vk::PipelineStageFlagBits::eEarlyFragmentTests
        | vk::PipelineStageFlagBits::eLateFragmentTests,
        vk::PipelineStageFlagBits::eTransfer,
        vk::DependencyFlagBits::eByRegion,
        {}, {},
        vk::ImageMemoryBarrier(
            vk::AccessFlagBits::eDepthStencilAttachmentWrite,
            vk::AccessFlagBits::eTransferRead,
            vk::ImageLayout::eDepthStencilAttachmentOptimal,
            vk::ImageLayout::eTransferSrcOptimal,
            VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
            *depthImage,
            { vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil, 0, 1, 0, 1 }
        )
    );
    cmdBuf.copyImageToBuffer(
        *depthImage, vk::ImageLayout::eTransferSrcOptimal,
        *depthPixelReadBuffer,
        vk::BufferImageCopy(
            0, // buffer offset
            0, 0, // some weird 2D or 3D offsets, idk
            vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eDepth, 0, 0, 1),
            { mousePos.x, mousePos.y, 0 },
            { 1, 1, 1 }
        )
    );
    cmdBuf.pipelineBarrier(
        vk::PipelineStageFlagBits::eTransfer,
        vk::PipelineStageFlagBits::eComputeShader,
        vk::DependencyFlagBits::eByRegion,
        {}, {},
        vk::ImageMemoryBarrier(
            vk::AccessFlagBits::eTransferRead,
            vk::AccessFlagBits::eShaderRead,
            vk::ImageLayout::eTransferSrcOptimal,
            vk::ImageLayout::eShaderReadOnlyOptimal,
            VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
            *depthImage,
            { vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil, 0, 1, 0, 1 }
        )
    );
}
