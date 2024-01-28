#include "trc/GBufferPass.h"

#include <glm/detail/type_half.hpp>

#include "trc/base/Barriers.h"



trc::GBufferPass::GBufferPass(
    const Device& device,
    GBuffer& _gBuffer)
    :
    RenderPass(makeVkRenderPass(device), NUM_SUBPASSES),
    gBuffer(_gBuffer),
    framebufferSize(gBuffer.getSize()),
    framebuffer([&] {
        std::vector<vk::ImageView> views{
            gBuffer.getImageView(GBuffer::eNormals),
            gBuffer.getImageView(GBuffer::eAlbedo),
            gBuffer.getImageView(GBuffer::eMaterials),
            gBuffer.getImageView(GBuffer::eDepth),
        };

        return Framebuffer(device, *renderPass, gBuffer.getSize(), {}, std::move(views));
    }()),
    clearValues({
        vk::ClearColorValue(std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 0.0f }),
        vk::ClearColorValue(std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 0.0f }),
        vk::ClearColorValue(std::array<ui32, 4>{ 0, 0, 0, 0 }),
        vk::ClearDepthStencilValue(1.0f, 0.0f),
    })
{
    setClearColor({ 0.2f, 0.5f, 1.0f, 1.0f });
}

void trc::GBufferPass::begin(
    vk::CommandBuffer cmdBuf,
    vk::SubpassContents subpassContents,
    FrameRenderState&)
{
    gBuffer.initFrame(cmdBuf);

    // Bring depth image into depthStencil layout
    barrier(cmdBuf, vk::ImageMemoryBarrier2{
        // 1st scope
        vk::PipelineStageFlagBits2::eBottomOfPipe,
        vk::AccessFlagBits2::eNone,
        // 2nd scope
        vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
        vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
        // Layout transition
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eDepthStencilAttachmentOptimal,
        VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
        *gBuffer.getImage(GBuffer::eDepth),
        { vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil, 0, 1, 0, 1 }
    });

    cmdBuf.beginRenderPass(
        vk::RenderPassBeginInfo(
            *renderPass,
            *framebuffer,
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

void trc::GBufferPass::end(vk::CommandBuffer cmdBuf)
{
    cmdBuf.endRenderPass();
}

void trc::GBufferPass::setClearColor(const vec4 c)
{
    clearValues[1] = vk::ClearColorValue(std::array<float, 4>{ c.r, c.g, c.b, c.a });
}

auto trc::GBufferPass::makeVkRenderPass(const Device& device)
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
        // Material parameters
        vk::AttachmentDescription(
            {}, vk::Format::eR8G8B8A8Unorm, vk::SampleCountFlagBits::e1,
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
            vk::ImageLayout::eUndefined, vk::ImageLayout::eShaderReadOnlyOptimal
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
