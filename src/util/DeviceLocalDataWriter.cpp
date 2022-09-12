#include "util/DeviceLocalDataWriter.h"

#include <vkb/Barriers.h>

#include "FrameRenderState.h"



trc::DeviceLocalDataWriter::DeviceLocalDataWriter(
    const vkb::Device& device,
    vkb::DeviceMemoryAllocator alloc)
    :
    device(device),
    alloc(std::move(alloc))
{
    updateData.emplace_back(new PersistentUpdateStructures);
}

void trc::DeviceLocalDataWriter::update(vk::CommandBuffer cmdBuf, FrameRenderState& state)
{
    PersistentUpdateStructures* frame{ nullptr };
    {
        std::scoped_lock lock(updateDataLock);

        // Get update information for current update
        frame = &getCurrentUpdateStruct();
        if (frame->empty()) return;

        // Append new update information storage
        updateData.emplace_back(new PersistentUpdateStructures);

        state.onRenderFinished([this, ptr=frame] {
            std::scoped_lock lock(updateDataLock);
            updateData.erase(
                std::ranges::find_if(updateData, [ptr](auto& p){ return p.get() == ptr; })
            );
        });
    } // mutex lock lifetime

    // Pre-write barriers
    if (!frame->preWriteBufferBarriers.empty() || !frame->preWriteImageBarriers.empty())
    {
        cmdBuf.pipelineBarrier(
            frame->preWriteBarrierSrcStageFlags,
            frame->preWriteBarrierDstStageFlags,
            vk::DependencyFlagBits::eByRegion,
            {}, frame->preWriteBufferBarriers, frame->preWriteImageBarriers
        );
    }

    // Copy data to buffers
    for (auto& write : frame->pendingBufferWrites)
    {
        cmdBuf.copyBuffer(
            *write.stagingBuffer, write.dstBuffer, write.copyRegion
        );
        vkb::bufferMemoryBarrier(
            cmdBuf,
            write.dstBuffer, write.copyRegion.dstOffset, write.copyRegion.size,
            vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eAllCommands,
            vk::AccessFlagBits::eTransferWrite,
            vk::AccessFlagBits::eMemoryWrite | vk::AccessFlagBits::eMemoryRead
        );
    }

    // Copy data to images
    for (auto& write : frame->pendingImageWrites)
    {
        cmdBuf.copyBufferToImage(
            *write.stagingBuffer,
            write.dstImage,
            vk::ImageLayout::eTransferDstOptimal,
            write.copyRegion
        );
        auto& r = write.copyRegion.imageSubresource;
        vkb::imageMemoryBarrier(
            cmdBuf,
            write.dstImage,
            vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eTransferDstOptimal,
            vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eAllCommands,
            vk::AccessFlagBits::eTransferWrite,
            vk::AccessFlagBits::eMemoryWrite | vk::AccessFlagBits::eMemoryRead,
            vk::ImageSubresourceRange(r.aspectMask, r.mipLevel, 1, r.baseArrayLayer, r.layerCount)
        );
    }

    // Post-write barriers
    if (!frame->postWriteBufferBarriers.empty() || !frame->postWriteImageBarriers.empty())
    {
        cmdBuf.pipelineBarrier(
            frame->postWriteBarrierSrcStageFlags,
            frame->postWriteBarrierDstStageFlags,
            vk::DependencyFlagBits::eByRegion,
            {}, frame->postWriteBufferBarriers, frame->postWriteImageBarriers
        );
    }
}

void trc::DeviceLocalDataWriter::write(
    vk::Buffer dst,
    size_t dstOffset,
    const void* src,
    size_t size)
{
    assert(dst);
    assert(src != nullptr);
    assert(size > 0);

    std::scoped_lock lock(updateDataLock);
    getCurrentUpdateStruct().pendingBufferWrites.push_back({
        dst,
        vkb::Buffer(
            device,
            size, src,
            vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible,
            alloc
        ),
        vk::BufferCopy(0, dstOffset, size)
    });
}

void trc::DeviceLocalDataWriter::write(
    vk::Image dst,
    vk::ImageSubresourceLayers subres,
    vk::Offset3D dstOffset,
    vk::Extent3D dstExtent,
    const void* src,
    size_t size)
{
    assert(dst);
    assert(src != nullptr);
    assert(size > 0);

    std::scoped_lock lock(updateDataLock);
    getCurrentUpdateStruct().pendingImageWrites.push_back({
        dst,
        vkb::Buffer(
            device,
            size, src,
            vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible,
            alloc
        ),
        vk::BufferImageCopy(0, 0, 0, subres, dstOffset, dstExtent)
    });
}

void trc::DeviceLocalDataWriter::barrierPreWrite(
    vk::PipelineStageFlags srcStageMask,
    vk::PipelineStageFlags dstStageMask,
    vk::BufferMemoryBarrier barrier)
{
    std::scoped_lock lock(updateDataLock);

    auto& data = getCurrentUpdateStruct();
    data.preWriteBufferBarriers.emplace_back(barrier);
    data.preWriteBarrierSrcStageFlags |= srcStageMask;
    data.preWriteBarrierDstStageFlags |= dstStageMask;
}

void trc::DeviceLocalDataWriter::barrierPostWrite(
    vk::PipelineStageFlags srcStageMask,
    vk::PipelineStageFlags dstStageMask,
    vk::BufferMemoryBarrier barrier)
{
    std::scoped_lock lock(updateDataLock);

    auto& data = getCurrentUpdateStruct();
    data.postWriteBufferBarriers.emplace_back(barrier);
    data.postWriteBarrierSrcStageFlags |= srcStageMask;
    data.postWriteBarrierDstStageFlags |= dstStageMask;
}

void trc::DeviceLocalDataWriter::barrierPreWrite(
    vk::PipelineStageFlags srcStageMask,
    vk::PipelineStageFlags dstStageMask,
    vk::ImageMemoryBarrier barrier)
{
    std::scoped_lock lock(updateDataLock);

    auto& data = getCurrentUpdateStruct();
    data.preWriteImageBarriers.emplace_back(barrier);
    data.preWriteBarrierSrcStageFlags |= srcStageMask;
    data.preWriteBarrierDstStageFlags |= dstStageMask;
}

void trc::DeviceLocalDataWriter::barrierPostWrite(
    vk::PipelineStageFlags srcStageMask,
    vk::PipelineStageFlags dstStageMask,
    vk::ImageMemoryBarrier barrier)
{
    std::scoped_lock lock(updateDataLock);

    auto& data = getCurrentUpdateStruct();
    data.postWriteImageBarriers.emplace_back(barrier);
    data.postWriteBarrierSrcStageFlags |= srcStageMask;
    data.postWriteBarrierDstStageFlags |= dstStageMask;
}

auto trc::DeviceLocalDataWriter::getCurrentUpdateStruct() -> PersistentUpdateStructures&
{
    assert(updateDataLock.try_lock() == false && "Mutex must have been acquired by caller!");
    assert(!updateData.empty());
    assert(updateData.back() != nullptr);

    return *updateData.back();
}



bool trc::DeviceLocalDataWriter::PersistentUpdateStructures::empty() const
{
    return pendingBufferWrites.empty() && pendingImageWrites.empty();
}
