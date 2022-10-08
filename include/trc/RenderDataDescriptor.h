#pragma once

#include "trc/base/Swapchain.h"
#include "trc/base/Buffer.h"

#include "trc/core/DescriptorProvider.h"
#include "trc/core/Camera.h"
#include "trc_util/Padding.h"

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
     *      vec2 mousePosition          (in pixels)
     *      vec2 swapchainResolution    (in pixels)
     */
    class GlobalRenderDataDescriptor : public DescriptorProviderInterface
    {
    public:
        GlobalRenderDataDescriptor(const Window& window);

        auto getDescriptorSetLayout() const noexcept -> vk::DescriptorSetLayout override;
        void bindDescriptorSet(
            vk::CommandBuffer cmdBuf,
            vk::PipelineBindPoint bindPoint,
            vk::PipelineLayout pipelineLayout,
            ui32 setIndex
        ) const override;

        void update(const Camera& camera);

    private:
        static constexpr vk::DeviceSize CAMERA_DATA_SIZE{
            util::pad(sizeof(mat4) * 4 + sizeof(vec4) * 2, 256u)
        };
        static constexpr vk::DeviceSize SWAPCHAIN_DATA_SIZE{ sizeof(vec2) * 2 };

        void createDescriptors();

        vk::UniqueDescriptorPool descPool;
        vk::UniqueDescriptorSetLayout descLayout;
        vk::UniqueDescriptorSet descSet;

        const Device& device;
        const Swapchain& swapchain;
        const ui32 BUFFER_SECTION_SIZE; // not static because it depends on physical device align

        /** Contains all descriptor data at dynamic offsets */
        Buffer buffer;
    };
} // namespace trc
