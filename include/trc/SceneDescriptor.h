#pragma once

#include <vkb/StaticInit.h>
#include <vkb/Buffer.h>

#include "DescriptorProvider.h"
#include <nc/functional/Maybe.h>
#include "PickableRegistry.h"

namespace trc
{
    class Scene;

    class SceneDescriptor
    {
    public:
        explicit SceneDescriptor(const Scene& scene);

        auto updatePicking() -> Maybe<ui32>;
        auto getProvider() const noexcept -> const DescriptorProviderInterface&;

        /**
         * The descriptor set layout is global for all SceneDescriptor
         * instances.
         */
        static auto getDescLayout() noexcept -> vk::DescriptorSetLayout;

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

        // The descriptor set layout is the same for all instances
        static inline vk::UniqueDescriptorSetLayout descLayout;
        static vkb::StaticInit _init;

        void createDescriptors(const Scene& scene);
        vk::UniqueDescriptorPool descPool;
        vk::UniqueDescriptorSet descSet;
        SceneDescriptorProvider provider{ *this };

        static constexpr vk::DeviceSize PICKING_BUFFER_SECTION_SIZE = 16;
        vkb::Buffer pickingBuffer;
        ui32 pickedObject{ NO_PICKABLE };
    };
} // namespace trc
