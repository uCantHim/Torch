#pragma once

#include "trc/Types.h"
#include "trc/base/Buffer.h"

namespace trc
{
    class ViewportDrawContext;

    class DepthReaderCallback
    {
    public:
        virtual ~DepthReaderCallback() noexcept = default;

        virtual auto getReadPosition() -> ivec2 = 0;
        virtual void onDepthRead(ivec2 pos, float depthValue) = 0;
    };

    class GBufferDepthReader
    {
    public:
        GBufferDepthReader(const Device& device,
                           s_ptr<DepthReaderCallback> callback,
                           vk::Image readImage,
                           const DeviceMemoryAllocator& alloc = DefaultDeviceMemoryAllocator{});

        void update(vk::CommandBuffer cmdBuf, ViewportDrawContext& ctx);

        static auto packedD24S8ToDepth(ui32 depthValueD24S8) -> float;

    private:
        s_ptr<DepthReaderCallback> callback;

        vk::Image readImage;
        Buffer depthPixelReadBuffer;
        ui32* depthBufMap;
    };
} // namespace trc
