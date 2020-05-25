#pragma once
#ifndef TRIANGLELIST_H
#define TRIANGLELIST_H

#include "vkb/Buffer.h"
#include "VulkanDrawable.h"
#include "RenderEnvironment.h"
#include "Settings.h"

class TriangleList
    : public VulkanDrawable<TriangleList>,
      public has_graphics_pipeline<TriangleList, STANDARD_PIPELINE, 0>
{
public:
    explicit TriangleList(size_t vertexCount);

    [[nodiscard]]
    auto recordCommandBuffer(uint32_t subpass, const vk::CommandBufferInheritanceInfo& inheritInfo)
        -> vk::CommandBuffer override;

private:
    vkb::FrameSpecificObject<vk::UniqueCommandBuffer> cmdBuf;

    const size_t vertexCount;
    vkb::DeviceLocalBuffer vertexBuffer;
};



#endif
