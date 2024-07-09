#pragma once

#include "trc/ray_tracing/AccelerationStructure.h"

namespace trc::rt
{
    class RaygenDescriptorPool
    {
    public:
        RaygenDescriptorPool(const Instance& instance, ui32 maxDescriptorSets);

        auto getTlasDescriptorSetLayout() const -> vk::DescriptorSetLayout;
        auto getImageDescriptorSetLayout() const -> vk::DescriptorSetLayout;

        auto allocateTlasDescriptorSet(const TLAS& tlas) -> vk::UniqueDescriptorSet;
        auto allocateImageDescriptorSet(vk::ImageView image) -> vk::UniqueDescriptorSet;

    private:
        const Device& device;

        vk::UniqueDescriptorPool pool;
        vk::UniqueDescriptorSetLayout tlasLayout;
        vk::UniqueDescriptorSetLayout imageLayout;
    };
} // namespace trc::rt
