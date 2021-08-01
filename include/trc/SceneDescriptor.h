#pragma once

#include <vkb/Buffer.h>

#include "Types.h"
#include "DescriptorProvider.h"
#include "PickableRegistry.h"

namespace trc
{
    class Window;
    class Instance;
    class Scene;

    class SceneDescriptor
    {
    public:
        explicit SceneDescriptor(const Window& window);

        void update(const Scene& scene);

        void updatePicking();
        auto getProvider() const noexcept -> const DescriptorProviderInterface&;

        /**
         * The descriptor set layout is global for all SceneDescriptor
         * instances.
         */
        auto getDescLayout() const noexcept -> vk::DescriptorSetLayout;

    private:
        class SceneDescriptorProvider : public DescriptorProviderInterface
        {
        public:
            explicit SceneDescriptorProvider(const SceneDescriptor& descriptor);

            auto getDescriptorSet() const noexcept -> vk::DescriptorSet override;
            auto getDescriptorSetLayout() const noexcept -> vk::DescriptorSetLayout override;
            void bindDescriptorSet(
                vk::CommandBuffer cmdBuf,
                vk::PipelineBindPoint bindPoint,
                vk::PipelineLayout pipelineLayout,
                ui32 setIndex
            ) const override;

        private:
            const SceneDescriptor& descriptor;
        };

        void createDescriptors(const Instance& instance);
        void writeDescriptors(const Instance& instance, const Scene& scene);

        const Window& window;

        vk::UniqueDescriptorSetLayout descLayout;
        vk::UniqueDescriptorPool descPool;
        vk::UniqueDescriptorSet descSet;
        SceneDescriptorProvider provider{ *this };

        static constexpr vk::DeviceSize PICKING_BUFFER_SECTION_SIZE = 16;
        vkb::Buffer pickingBuffer;
        ui32 currentlyPicked{ NO_PICKABLE };
    };
} // namespace trc
