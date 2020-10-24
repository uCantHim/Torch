#pragma once

#include "DescriptorProvider.h"

namespace trc
{
    class Scene;

    class SceneDescriptor
    {
    public:
        explicit SceneDescriptor(const Scene& scene);

        /**
         * The descriptor set is per-scene
         */
        auto getDescSet() const noexcept -> vk::DescriptorSet;

        /**
         * The descriptor set layout is global for all SceneDescriptor
         * instances.
         */
        static auto getDescLayout() noexcept -> vk::DescriptorSetLayout;

    private:
        // The descriptor set layout is the same for all instances
        static inline vk::UniqueDescriptorSetLayout descLayout;
        static vkb::StaticInit _init;

        void createDescriptors(const Scene& scene);
        vk::UniqueDescriptorPool descPool;
        vk::UniqueDescriptorSet descSet;
    };
} // namespace trc
