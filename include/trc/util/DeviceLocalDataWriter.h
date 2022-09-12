#pragma once

#include <vector>
#include <mutex>

#include <vkb/Memory.h>
#include <vkb/Buffer.h>

#include "VulkanInclude.h"
#include "Types.h"

namespace trc
{
    class FrameRenderState;

    class DeviceLocalDataWriter
    {
    public:
        explicit DeviceLocalDataWriter(const vkb::Device& device,
                                       vkb::DeviceMemoryAllocator alloc
                                           = vkb::DefaultDeviceMemoryAllocator{});

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
        const vkb::Device& device;
        vkb::DeviceMemoryAllocator alloc;

        struct BufferWrite
        {
            vk::Buffer dstBuffer;
            vkb::Buffer stagingBuffer;

            vk::BufferCopy copyRegion;
        };

        struct ImageWrite
        {
            vk::Image dstImage;
            vkb::Buffer stagingBuffer;

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
