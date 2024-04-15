#pragma once

#include "trc/base/Swapchain.h"
#include "trc/base/Buffer.h"

#include "trc/core/DescriptorProvider.h"
#include "trc/Camera.h"

namespace trc
{
    class Window;

    /**
     * @brief Provides global, renderer-specific data
     *
     * Contains the following data:
     *
     * - binding 0:
     *      mat4 currentViewMatrix
     *      mat4 currentProjMatrix
     *      mat4 currentInverseViewMatrix
     *      mat4 currentInverseProjMatrix
     *
     * - binding 1:
     *      vec2 renderAreaSize    (in pixels)
     */
    class GlobalRenderDataDescriptor
    {
    public:
        class DescriptorSet : public DescriptorProviderInterface
        {
        public:
            void update(const Camera& camera);

            void bindDescriptorSet(vk::CommandBuffer cmdBuf,
                                   vk::PipelineBindPoint bindPoint,
                                   vk::PipelineLayout pipelineLayout,
                                   ui32 setIndex
                                   ) const override;

        private:
            friend GlobalRenderDataDescriptor;
            DescriptorSet(const GlobalRenderDataDescriptor* parent,
                          vk::UniqueDescriptorSet set,
                          ui32 bufferOffset)
                : parent(parent), descSet(std::move(set)), bufferOffset(bufferOffset)
            {}

            const GlobalRenderDataDescriptor* parent;
            vk::UniqueDescriptorSet descSet;
            ui32 bufferOffset;
        };

        GlobalRenderDataDescriptor(const Device& device, ui32 maxDescriptorSets);

        auto getDescriptorSetLayout() const noexcept -> vk::DescriptorSetLayout;
        auto makeDescriptorSet() const -> DescriptorSet;

    private:
        static constexpr vk::DeviceSize kCameraDataSize{ sizeof(mat4) * 4 };
        static constexpr vk::DeviceSize kViewportDataSize{ sizeof(vec2) };

        void createDescriptors();

        vk::UniqueDescriptorPool descPool;
        vk::UniqueDescriptorSetLayout descLayout;

        const Device& device;
        const ui32 kBufferSectionSize; // not static because it depends on physical device align
        const ui32 kMaxDescriptorSets;

        /** Contains all descriptor data at dynamic offsets */
        Buffer buffer;
    };
} // namespace trc
