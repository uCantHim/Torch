#pragma once

#include "VulkanInclude.h"

namespace vkb
{
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
} // namespace vkb
