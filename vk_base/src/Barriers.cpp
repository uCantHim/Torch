#include "Barriers.h"



void vkb::barrier(vk::CommandBuffer cmdBuf, vk::ImageMemoryBarrier2 imageBarrier)
{
    cmdBuf.pipelineBarrier2({ vk::DependencyFlagBits::eByRegion, {}, {}, imageBarrier });
}

void vkb::barrier(vk::CommandBuffer cmdBuf, vk::BufferMemoryBarrier2 bufferBarrier)
{
    cmdBuf.pipelineBarrier2({ vk::DependencyFlagBits::eByRegion, {}, bufferBarrier, {} });
}

void vkb::barrier(vk::CommandBuffer cmdBuf, vk::MemoryBarrier2 memoryBarrier)
{
    cmdBuf.pipelineBarrier2({ vk::DependencyFlagBits::eByRegion, memoryBarrier, {}, {} });
}

void vkb::imageMemoryBarrier(
    vk::CommandBuffer cmdBuf,
    vk::Image image,
    vk::ImageLayout from,
    vk::ImageLayout to,
    vk::PipelineStageFlags srcStages,
    vk::PipelineStageFlags dstStages,
    vk::AccessFlags srcAccess,
    vk::AccessFlags dstAccess,
    vk::ImageSubresourceRange subRes)
{
    cmdBuf.pipelineBarrier(
        srcStages, dstStages,
        vk::DependencyFlagBits::eByRegion,
        {},
        {},
        vk::ImageMemoryBarrier(
            srcAccess, dstAccess,
            from, to,
            VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
            image, std::move(subRes)
        )
    );
}

void vkb::bufferMemoryBarrier(
    vk::CommandBuffer cmdBuf,
    vk::Buffer buffer,
    vk::DeviceSize offset,
    vk::DeviceSize size,
    vk::PipelineStageFlags srcStages,
    vk::PipelineStageFlags dstStages,
    vk::AccessFlags srcAccess,
    vk::AccessFlags dstAccess)
{
    cmdBuf.pipelineBarrier(
        srcStages, dstStages,
        vk::DependencyFlagBits::eByRegion,
        {},
        vk::BufferMemoryBarrier(
            srcAccess, dstAccess,
            VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
            buffer, offset, size
        ),
        {}
    );
}

void vkb::memoryBarrier(
    vk::CommandBuffer cmdBuf,
    vk::PipelineStageFlags srcStages,
    vk::PipelineStageFlags dstStages,
    vk::AccessFlags srcAccess,
    vk::AccessFlags dstAccess)
{
    cmdBuf.pipelineBarrier(
        srcStages, dstStages,
        vk::DependencyFlagBits::eByRegion,
        vk::MemoryBarrier(srcAccess, dstAccess),
        {},
        {}
    );
}
