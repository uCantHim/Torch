#pragma once

#include <vkb/VulkanBase.h>
#include <vkb/Image.h>
#include <vkb/Buffer.h>
#include <vkb/FrameSpecificObject.h>
#include <vkb/event/EventHandler.h>
#include <vkb/event/WindowEvents.h>

#include "RenderStage.h"
#include "SceneDescriptor.h"
#include "CommandCollector.h"

#include "RenderPassDeferred.h"
#include "RenderPassShadow.h"
#include "LightRegistry.h"

namespace trc
{
    class DeferredStage : public RenderStage
    {
    public:
        DeferredStage() : RenderStage(RenderPassDeferred::NUM_SUBPASSES) {}
    };

    class Renderer;

    // TODO
    extern auto init() -> std::unique_ptr<Renderer>;

    /**
     * @brief Destroy all resources allocated by Torch
     *
     * Does call vkb::terminate for you!
     *
     * You should release all of your resources before calling this
     * function.
     */
    extern void terminate();

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

        static constexpr vk::DeviceSize CAMERA_DATA_SIZE{ sizeof(mat4) * 4 };
        static constexpr vk::DeviceSize SWAPCHAIN_DATA_SIZE{ sizeof(vec2) * 2 };
        const ui32 BUFFER_SECTION_SIZE; // not static because depends on physical device align
        vkb::Buffer buffer;
    };

    /**
     * @brief The heart of the Torch rendering pipeline
     */
    class Renderer
    {
    public:
        Renderer();

        Renderer(const Renderer&) = delete;
        Renderer(Renderer&&) = delete;
        Renderer& operator=(const Renderer&) = delete;
        Renderer& operator=(Renderer&&) = delete;
        ~Renderer() = default;

        void drawFrame(Scene& scene, const Camera& camera);

        void addStage(RenderStage::ID stage, ui32 priority);

        auto getDefaultDeferredStageId() const noexcept -> RenderStage::ID;
        auto getDefaultDeferredStage() const noexcept -> DeferredStage&;
        auto getDefaultShadowStageId() const noexcept -> RenderStage::ID;
        auto getDefaultShadowStage() const noexcept -> ShadowStage&;

        auto getDeferredRenderPassId() const noexcept -> RenderPass::ID;
        auto getDeferredRenderPass() const noexcept -> const RenderPassDeferred&;

        auto getGlobalDataDescriptor() const noexcept -> const GlobalRenderDataDescriptor&;
        auto getGlobalDataDescriptorProvider() const noexcept -> const DescriptorProviderInterface&;
        auto getSceneDescriptorProvider() const noexcept -> const DescriptorProviderInterface&;
        auto getShadowDescriptorProvider() const noexcept -> const DescriptorProviderInterface&;

    private:
        // Initialize render stages
        static vkb::StaticInit _init;
        vkb::UniqueListenerId<vkb::PreSwapchainRecreateEvent> preRecreateListener;
        vkb::UniqueListenerId<vkb::SwapchainRecreateEvent> postRecreateListener;

        // Synchronization
        void waitForAllFrames(ui64 timeoutNs = UINT64_MAX);
        void createSemaphores();
        vkb::FrameSpecificObject<vk::UniqueSemaphore> imageAcquireSemaphores;
        vkb::FrameSpecificObject<vk::UniqueSemaphore> renderFinishedSemaphores;
        vkb::FrameSpecificObject<vk::UniqueFence> frameInFlightFences;

        // Render passes and -stages (render passes have nothing to do with stages!)
        RenderPass::ID deferredPassId;

        std::vector<std::pair<RenderStage::ID, ui32>> renderStages;
        std::vector<CommandCollector> commandCollectors;

        // Other things
        GlobalRenderDataDescriptor globalDataDescriptor;
        // A single descriptor provider. The descriptor set is switched to
        // that of the current scene every frame.
        DescriptorProvider sceneDescriptorProvider{ SceneDescriptor::getDescLayout(), {} };
        // A single descriptor provider. The descriptor sets are switched
        // to those of the current scene every frame.
        // TODO: implement a more efficient provider wrapper so that I don't
        // have to recreate the list of sets every frame
        FrameSpecificDescriptorProvider shadowDescriptorProvider{
            _ShadowDescriptor::getDescLayout(), {}
        };

        vkb::DeviceLocalBuffer fullscreenQuadVertexBuffer;
    };
}
