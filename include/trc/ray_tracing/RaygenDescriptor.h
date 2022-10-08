#pragma once

#include "trc/base/Image.h"
#include "trc/base/FrameClock.h"
#include "trc/base/FrameSpecificObject.h"

#include "trc/ray_tracing/AccelerationStructure.h"

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
                                                FrameSpecific<vk::ImageView> outputImageView)
            -> FrameSpecific<vk::UniqueDescriptorSet>;

    private:
        const Device& device;

        vk::UniqueDescriptorPool pool;
        vk::UniqueDescriptorSetLayout layout;
    };
} // namespace trc::rt
