#pragma once

#include <vkb/Image.h>
#include <vkb/FrameClock.h>
#include <vkb/FrameSpecificObject.h>

#include "AccelerationStructure.h"

namespace trc::rt
{
    class RaygenDescriptorPool
    {
    public:
        RaygenDescriptorPool(const Instance& instance, ui32 maxDescriptorSets);

        auto getDescriptorSetLayout() const -> vk::DescriptorSetLayout;

        auto allocateDescriptorSet(const TLAS& tlas,
                                   vk::ImageView outputImageView)
            -> vk::UniqueDescriptorSet;

        auto allocateFrameSpecificDescriptorSet(const TLAS& tlas,
                                                vkb::FrameSpecific<vk::ImageView> outputImageView)
            -> vkb::FrameSpecific<vk::UniqueDescriptorSet>;

    private:
        const vkb::Device& device;

        vk::UniqueDescriptorPool pool;
        vk::UniqueDescriptorSetLayout layout;
    };
} // namespace trc::rt
