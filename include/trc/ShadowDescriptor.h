#pragma once

#include <vkb/Image.h>
#include <vkb/StaticInit.h>
#include <vkb/FrameSpecificObject.h>

#include "TorchResources.h"
#include "LightRegistry.h"
#include "DescriptorProvider.h"

namespace trc
{
    class Window;
    class LightRegistry;

    class ShadowDescriptor
    {
        friend class LightRegistry;

    public:
        explicit ShadowDescriptor(const Window& window);

        void update(const LightRegistry& lights);

        auto getProvider() const noexcept -> const DescriptorProviderInterface&;

        /**
         * The descriptor set layout is global for all SceneDescriptor
         * instances.
         */
        auto getDescLayout() const noexcept -> vk::DescriptorSetLayout;

    private:
        const Window& window;

        // The descriptor set layout is the same for all instances
        vkb::Image dummyShadowImage;
        vk::UniqueImageView dummyImageView;

        void createDescriptors(const Window& window, ui32 maxShadowMaps);
        void writeDescriptors(const LightRegistry& lightRegistry);

        vk::UniqueDescriptorSetLayout descLayout;
        vk::UniqueDescriptorPool descPool;
        vkb::FrameSpecificObject<vk::UniqueDescriptorSet> descSets;
        FrameSpecificDescriptorProvider provider;
    };
} // namespace trc
