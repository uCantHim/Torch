#pragma once

#include "DescriptorProvider.h"

namespace trc
{
    class Scene;

    class SceneDescriptor
    {
    public:
        SceneDescriptor();

        auto getProvider() const noexcept -> const DescriptorProviderInterface&;

        void updateActiveScene(const Scene& scene) const noexcept;

    private:
        void createDescriptors();

        vk::UniqueDescriptorPool descPool;
        vk::UniqueDescriptorSetLayout descLayout;
        vk::UniqueDescriptorSet descSet;
        DescriptorProvider provider{ {}, {} };
    };
} // namespace trc
