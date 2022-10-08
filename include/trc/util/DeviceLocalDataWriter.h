#pragma once

#include <vector>
#include <mutex>

#include "trc/base/Memory.h"
#include "trc/base/Buffer.h"

#include "trc/VulkanInclude.h"
#include "trc/Types.h"

namespace trc
{
    class FrameRenderState;

    class DeviceLocalDataWriter
    {
    public:
        explicit DeviceLocalDataWriter(const Device& device,
                                       DeviceMemoryAllocator alloc
                                           = DefaultDeviceMemoryAllocator{});

        void update(vk::CommandBuffer cmdBuf, FrameRenderState& state);

        void write(vk::Buffer dst,
                   size_t dstOffset,
                   const void* src,
                   size_t size);

        /**
         * At the time of the write, the destination image *must* be in the
         * `eTransferDstOptimal` layout.
         */
        void write(vk::Image dst,
                   vk::ImageSubresourceLayers subres,
                   vk::Offset3D dstOffset,
                   vk::Extent3D dstExtent,
                   const void* src,
                   size_t size);

        void barrierPreWrite(vk::PipelineStageFlags srcStageMask,
                             vk::PipelineStageFlags dstStageMask,
                             vk::BufferMemoryBarrier barrier);
        void barrierPostWrite(vk::PipelineStageFlags srcStageMask,
                              vk::PipelineStageFlags dstStageMask,
                              vk::BufferMemoryBarrier barrier);
        void barrierPreWrite(vk::PipelineStageFlags srcStageMask,
                             vk::PipelineStageFlags dstStageMask,
                             vk::ImageMemoryBarrier barrier);
        void barrierPostWrite(vk::PipelineStageFlags srcStageMask,
                              vk::PipelineStageFlags dstStageMask,
                              vk::ImageMemoryBarrier barrier);

    private:
        const Device& device;
        DeviceMemoryAllocator alloc;

        struct BufferWrite
        {
            vk::Buffer dstBuffer;
            Buffer stagingBuffer;

            vk::BufferCopy copyRegion;
        };

        struct ImageWrite
        {
            vk::Image dstImage;
            Buffer stagingBuffer;

            vk::BufferImageCopy copyRegion;
        };

        struct PersistentUpdateStructures
        {
            bool empty() const;

            std::vector<BufferWrite> pendingBufferWrites;
            std::vector<vk::BufferMemoryBarrier> preWriteBufferBarriers;
            std::vector<vk::BufferMemoryBarrier> postWriteBufferBarriers;

            std::vector<ImageWrite> pendingImageWrites;
            std::vector<vk::ImageMemoryBarrier> preWriteImageBarriers;
            std::vector<vk::ImageMemoryBarrier> postWriteImageBarriers;

            vk::PipelineStageFlags preWriteBarrierSrcStageFlags;
            vk::PipelineStageFlags preWriteBarrierDstStageFlags;
            vk::PipelineStageFlags postWriteBarrierSrcStageFlags;
            vk::PipelineStageFlags postWriteBarrierDstStageFlags;
        };

        auto getCurrentUpdateStruct() -> PersistentUpdateStructures&;

        std::mutex updateDataLock;
        std::vector<u_ptr<PersistentUpdateStructures>> updateData;
    };
} // namespace trc
