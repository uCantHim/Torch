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

    /**
     * binding 0: Lights (uniform buffer)
     * binding 1: Picking information (storage buffer)
     */
    class SceneDescriptor
    {
    public:
        explicit SceneDescriptor(const Window& window);

        void update(const Scene& scene);

        void updatePicking();
        auto getProvider() const noexcept -> const DescriptorProviderInterface&;

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

        void createDescriptors();
        void writeDescriptors();

        const Window& window;
        const vkb::Device& device;

        vk::UniqueDescriptorSetLayout descLayout;
        vk::UniqueDescriptorPool descPool;
        vk::UniqueDescriptorSet descSet;
        SceneDescriptorProvider provider{ *this };

        vkb::Buffer lightBuffer;
        ui8* lightBufferMap;  // Persistent mapping

        static constexpr vk::DeviceSize PICKING_BUFFER_SECTION_SIZE = 16;
        vkb::Buffer pickingBuffer;
        ui32 currentlyPicked{ NO_PICKABLE };
    };
} // namespace trc
