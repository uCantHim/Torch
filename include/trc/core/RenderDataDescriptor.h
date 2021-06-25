#pragma once

#include <vkb/basics/Swapchain.h>
#include <vkb/Buffer.h>

#include "DescriptorProvider.h"
#include "Camera.h"
#include "utils/Util.h"

namespace trc
{
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
     *      vec2 mousePosition          (in pixels)
     *      vec2 swapchainResolution    (in pixels)
     */
    class GlobalRenderDataDescriptor
    {
    public:
        GlobalRenderDataDescriptor();

        auto getProvider() const noexcept -> const DescriptorProviderInterface&;

        void updateCameraMatrices(const Camera& camera);
        void updateSwapchainData(const vkb::Swapchain& swapchain);

    private:
        /** Calculates dynamic offsets into the buffer */
        class RenderDataDescriptorProvider : public DescriptorProviderInterface
        {
        public:
            RenderDataDescriptorProvider(const GlobalRenderDataDescriptor& desc);

            auto getDescriptorSet() const noexcept -> vk::DescriptorSet override;
            auto getDescriptorSetLayout() const noexcept -> vk::DescriptorSetLayout override;
            void bindDescriptorSet(
                vk::CommandBuffer cmdBuf,
                vk::PipelineBindPoint bindPoint,
                vk::PipelineLayout pipelineLayout,
                ui32 setIndex
            ) const override;

        private:
            const GlobalRenderDataDescriptor& descriptor;
        };

        void createResources();
        void createDescriptors();

        vk::UniqueDescriptorPool descPool;
        vk::UniqueDescriptorSetLayout descLayout;
        vk::UniqueDescriptorSet descSet;
        RenderDataDescriptorProvider provider;

        static constexpr vk::DeviceSize CAMERA_DATA_SIZE{
            util::pad(sizeof(mat4) * 4 + sizeof(vec4) * 2, 256u)
        };
        static constexpr vk::DeviceSize SWAPCHAIN_DATA_SIZE{ sizeof(vec2) * 2 };
        const ui32 BUFFER_SECTION_SIZE; // not static because depends on physical device align
        vkb::Buffer buffer;
    };
} // namespace trc
