#pragma once

#include <vkb/VulkanBase.h>
#include <vkb/Image.h>
#include <vkb/Buffer.h>
#include <vkb/FrameSpecificObject.h>

#include "Scene.h"
#include "CommandCollector.h"
#include "RenderStage.h"
#include "PipelineDefinitions.h"

#include "RenderPassShadow.h" // TODO: Rename to RenderStageShadow.h
#include "RenderStageDeferred.h"

namespace trc
{
    class Renderer : public vkb::SwapchainDependentResource
    {
    public:
        Renderer();

        void drawFrame(Scene& scene, const Camera& camera);

        void addStage(RenderStage::ID stage, ui32 priority);

    private:
        // Initialize render stages
        static inline vkb::StaticInit _render_stages_init{
            []() {
                RenderStage::create<DeferredStage>(internal::RenderStages::eDeferred);
                RenderStage::create<ShadowStage>(internal::RenderStages::eShadow);
            },
            []() {}
        };

        void signalRecreateRequired() override;
        void recreate(vkb::Swapchain& swapchain) override;
        void signalRecreateFinished() override;

        // Synchronization
        void waitForAllFrames(ui64 timeoutNs = UINT64_MAX);
        void createSemaphores();
        vkb::FrameSpecificObject<vk::UniqueSemaphore> imageAcquireSemaphores;
        vkb::FrameSpecificObject<vk::UniqueSemaphore> renderFinishedSemaphores;
        vkb::FrameSpecificObject<vk::UniqueFence> frameInFlightFences;

        // Render stages
        std::vector<std::pair<RenderStage::ID, ui32>> renderStages;
        std::vector<CommandCollector> commandCollectors;

        // Other things
        vkb::DeviceLocalBuffer fullscreenQuadVertexBuffer;
    };

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
     *      vec2 mousePosition          (specified in integer pixels)
     *      vec2 swapchainResolution    (in pixels)
     */
    class GlobalRenderDataDescriptor
    {
    public:
        static auto getProvider() noexcept -> const DescriptorProviderInterface&;

        static void updateCameraMatrices(const Camera& camera);
        static void updateSwapchainData(const vkb::Swapchain& swapchain);

    private:
        static void vulkanStaticInit();
        static void vulkanStaticDestroy();
        static inline vkb::StaticInit _init{
            vulkanStaticInit, vulkanStaticDestroy
        };

        /** Calculates dynamic offsets into the buffer */
        class RenderDataDescriptorProvider : public DescriptorProviderInterface
        {
        public:
            auto getDescriptorSet() const noexcept -> vk::DescriptorSet override;
            auto getDescriptorSetLayout() const noexcept -> vk::DescriptorSetLayout override;
            void bindDescriptorSet(
                vk::CommandBuffer cmdBuf,
                vk::PipelineBindPoint bindPoint,
                vk::PipelineLayout pipelineLayout,
                ui32 setIndex
            ) const override;
        };

        static inline vk::UniqueDescriptorPool descPool;
        static inline vk::UniqueDescriptorSetLayout descLayout;
        static inline vk::UniqueDescriptorSet descSet;
        static inline RenderDataDescriptorProvider provider;

        static inline ui32 BUFFER_SECTION_SIZE{ 0 };
        static constexpr vk::DeviceSize CAMERA_DATA_SIZE{ sizeof(mat4) * 4 };
        static constexpr vk::DeviceSize SWAPCHAIN_DATA_SIZE{ sizeof(vec2) * 2 };
        static inline vkb::Buffer buffer;
    };
}
