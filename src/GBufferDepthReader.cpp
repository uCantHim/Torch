#include "trc/GBufferDepthReader.h"

#include "trc/base/Barriers.h"

#include "trc/core/Window.h"



trc::GBufferDepthReader::GBufferDepthReader(
    const Device& device,
    std::function<vec2()> mousePosGetter,
    GBuffer& _gBuffer)
    :
    getMousePos(std::move(mousePosGetter)),
    gBuffer(_gBuffer),
    depthPixelReadBuffer(
        device,
        sizeof(float),
        vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eDeviceLocal
        | vk::MemoryPropertyFlagBits::eHostCoherent
    )
{
}

void trc::GBufferDepthReader::update(vk::CommandBuffer cmdBuf, FrameRenderState&)
{
    readDepthAtMousePos(cmdBuf);
}

auto trc::GBufferDepthReader::getMouseDepth() const noexcept -> float
{
    const ui32 depthValueD24S8 = depthBufMap[0];

    // Don't ask me why 16 bit here, I think it should be 24. The result is
    // correct when we use 65536 as depth 1.0 (maximum depth) though.
    constexpr float maxFloat16 = 65536.0f;  // 2^16
    return static_cast<float>(depthValueD24S8 >> 8) / maxFloat16;
}

void trc::GBufferDepthReader::readDepthAtMousePos(vk::CommandBuffer cmdBuf)
{
    Image& depthImage = gBuffer.getImage(GBuffer::eDepth);
    const ivec2 mousePos = glm::clamp(getMousePos(), vec2(0), vec2(depthImage.getSize()) - 1.0f);

    imageMemoryBarrier(
        cmdBuf,
        *depthImage,
        vk::ImageLayout::eShaderReadOnlyOptimal,
        vk::ImageLayout::eTransferSrcOptimal,
        vk::PipelineStageFlagBits::eEarlyFragmentTests
            | vk::PipelineStageFlagBits::eLateFragmentTests,
        vk::PipelineStageFlagBits::eTransfer,
        vk::AccessFlagBits::eDepthStencilAttachmentWrite,
        vk::AccessFlagBits::eTransferRead,
        { vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil, 0, 1, 0, 1 }
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
    imageMemoryBarrier(
        cmdBuf,
        *depthImage,
        vk::ImageLayout::eTransferSrcOptimal,
        vk::ImageLayout::eShaderReadOnlyOptimal,
        vk::PipelineStageFlagBits::eTransfer,
        vk::PipelineStageFlagBits::eComputeShader,
        vk::AccessFlagBits::eTransferRead,
        vk::AccessFlagBits::eShaderWrite,
        { vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil, 0, 1, 0, 1 }
    );
}
