#pragma once
#ifndef CUBE_H
#define CUBE_H

#include "vkb/FrameSpecificObject.h"
#include "vkb/Buffer.h"
#include "VulkanDrawable.h"

#include "Settings.h"

constexpr uint32_t CUBE_PIPELINE = 1;

class Quad
    : public VulkanDrawable<Quad>,
      public has_graphics_pipeline<Quad, STANDARD_PIPELINE, 0>
{
public:
    Quad();

    auto recordCommandBuffer(uint32_t subpass, const vk::CommandBufferInheritanceInfo& inheritInfo)
        -> vk::CommandBuffer override;

private:
    vkb::FrameSpecificObject<vk::UniqueCommandBuffer> commandBuffer;
    vkb::DeviceLocalBuffer geometryBuffer;
    vkb::DeviceLocalBuffer indexBuffer;
};

#endif
