#include "trc/GBufferDepthReader.h"

#include "trc/core/Frame.h"
#include "trc/core/RenderPipelineTasks.h"



trc::GBufferDepthReader::GBufferDepthReader(
    const Device& device,
    s_ptr<DepthReaderCallback> _callback,
    vk::Image image,
    const DeviceMemoryAllocator& alloc)
    :
    callback(std::move(_callback)),
    readImage(image),
    depthPixelReadBuffer(
        device,
        sizeof(float),
        vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eDeviceLocal
        | vk::MemoryPropertyFlagBits::eHostCoherent,
        alloc
    ),
    depthBufMap(depthPixelReadBuffer.map<ui32*>())
{
}

void trc::GBufferDepthReader::update(vk::CommandBuffer cmdBuf, ViewportDrawContext& ctx)
{
    if (callback == nullptr) {
        return;
    }

    const ivec2 readPos = callback->getReadPosition();

    ctx.deps().consume(ImageAccess{
        readImage,
        { vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil, 0, 1, 0, 1 },
        vk::PipelineStageFlagBits2::eTransfer,
        vk::AccessFlagBits2::eTransferRead,
        vk::ImageLayout::eTransferSrcOptimal,
    });
    cmdBuf.copyImageToBuffer(
        readImage,
        vk::ImageLayout::eTransferSrcOptimal,
        *depthPixelReadBuffer,
        vk::BufferImageCopy(
            0, // buffer offset
            0, 0, // some weird 2D or 3D offsets, idk
            vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eDepth, 0, 0, 1),
            { readPos.x, readPos.y, 0 },
            { 1, 1, 1 }
        )
    );

    ctx.frame().onRenderFinished([this, readPos]{
        if (callback != nullptr) {
            callback->onDepthRead(readPos, packedD24S8ToDepth(*depthBufMap));
        }
    });
}

auto trc::GBufferDepthReader::packedD24S8ToDepth(ui32 depthValueD24S8) -> float
{
    // Don't ask me why 16 bit here, I think it should be 24. The result is
    // correct when we use 65536 as depth 1.0 (maximum depth) though.
    constexpr float maxFloat16 = 65536.0f;  // 2^16

    return static_cast<float>(depthValueD24S8 >> 8) / maxFloat16;
}
