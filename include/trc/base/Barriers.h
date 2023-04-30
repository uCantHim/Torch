#pragma once

#include "trc/VulkanInclude.h"

namespace trc
{
    void barrier(vk::CommandBuffer cmdBuf, const vk::ImageMemoryBarrier2& imageBarrier);
    void barrier(vk::CommandBuffer cmdBuf, const vk::BufferMemoryBarrier2& bufferBarrier);
    void barrier(vk::CommandBuffer cmdBuf, const vk::MemoryBarrier2& memoryBarrier);

    void imageMemoryBarrier(vk::CommandBuffer cmdBuf,
                            vk::Image image,
                            vk::ImageLayout from,
                            vk::ImageLayout to,
                            vk::PipelineStageFlags srcStages,
                            vk::PipelineStageFlags dstStages,
                            vk::AccessFlags srcAccess,
                            vk::AccessFlags dstAccess,
                            vk::ImageSubresourceRange subRes);

    void bufferMemoryBarrier(vk::CommandBuffer cmdBuf,
                             vk::Buffer buffer,
                             vk::DeviceSize offset,
                             vk::DeviceSize size,
                             vk::PipelineStageFlags srcStages,
                             vk::PipelineStageFlags dstStages,
                             vk::AccessFlags srcAccess,
                             vk::AccessFlags dstAccess);

    void memoryBarrier(vk::CommandBuffer cmdBuf,
                       vk::PipelineStageFlags srcStages,
                       vk::PipelineStageFlags dstStages,
                       vk::AccessFlags srcAccess,
                       vk::AccessFlags dstAccess);
} // namespace trc
