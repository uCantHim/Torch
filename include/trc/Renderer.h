#pragma once

#include <vkb/VulkanBase.h>
#include <vkb/Image.h>
#include <vkb/Buffer.h>
#include <vkb/FrameSpecificObject.h>
#include <vkb/event/EventHandler.h>
#include <vkb/event/WindowEvents.h>

#include "TorchResources.h"
#include "RenderStage.h"
#include "RenderDataDescriptor.h"
#include "SceneDescriptor.h"
#include "CommandCollector.h"

#include "RenderPassDeferred.h"
#include "RenderPassShadow.h"
#include "LightRegistry.h"

namespace trc
{
    class Renderer;
    class Scene;

    struct RendererCreateInfo
    {
        vkb::Swapchain* swapchain;
        ui32 maxTransparentFragsPerPixel{ 3 };
    };

    struct TorchInitInfo
    {
        RendererCreateInfo rendererInfo;
    };

    /**
     * @brief Initialize all required Torch resources
     */
    extern auto init(const TorchInitInfo& info = {}) -> std::unique_ptr<Renderer>;

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
     * @brief The heart of the Torch rendering pipeline
     */
    class Renderer
    {
    public:
        explicit Renderer(RendererCreateInfo info = {});

        Renderer(const Renderer&) = delete;
        Renderer(Renderer&&) = delete;
        Renderer& operator=(const Renderer&) = delete;
        Renderer& operator=(Renderer&&) = delete;
        ~Renderer() = default;

        void drawFrame(Scene& scene, const Camera& camera);

        void enableRenderStageType(RenderStageType::ID stageType, i32 priority);
        void addRenderStage(RenderStageType::ID type, RenderStage& stage);
        void removeRenderStage(RenderStageType::ID type, RenderStage& stage);

        auto getDefaultDeferredStage() const noexcept -> const DeferredStage&;
        auto getDeferredRenderPass() const noexcept -> const RenderPassDeferred&;

        auto getGlobalDataDescriptor() const noexcept -> const GlobalRenderDataDescriptor&;
        auto getGlobalDataDescriptorProvider() const noexcept -> const DescriptorProviderInterface&;
        auto getSceneDescriptorProvider() const noexcept -> const DescriptorProviderInterface&;
        auto getShadowDescriptorProvider() const noexcept -> const DescriptorProviderInterface&;

    private:
        /**
         * @brief Wraps the scene descriptors into one object
         *
         * This is necessary because only a single descriptor provider is
         * stored in pipelines but the provider changes when the scene
         * changes. I can store a single DescriptorProviderWrapper object
         * in all pipelines and switch the scene descriptor every frame
         * transprently to the outside.
         */
        class DescriptorProviderWrapper : public DescriptorProviderInterface
        {
        public:
            explicit DescriptorProviderWrapper(vk::DescriptorSetLayout staticLayout);

            auto getDescriptorSet() const noexcept -> vk::DescriptorSet override;
            auto getDescriptorSetLayout() const noexcept -> vk::DescriptorSetLayout override;
            void bindDescriptorSet(
                vk::CommandBuffer cmdBuf,
                vk::PipelineBindPoint bindPoint,
                vk::PipelineLayout pipelineLayout,
                ui32 setIndex
            ) const override;

            void setWrappedProvider(const DescriptorProviderInterface& wrapped) noexcept;

        private:
            // TODO: Maybe use null descriptor here instead of
            // `if (nullptr)` every time
            const DescriptorProviderInterface* provider{ nullptr };

            // Use a static descriptor set layout. All exchangable
            // providers must have one.
            const vk::DescriptorSetLayout descLayout;
        };

        vkb::UniqueListenerId<vkb::PreSwapchainRecreateEvent> preRecreateListener;
        vkb::UniqueListenerId<vkb::SwapchainRecreateEvent> postRecreateListener;

        // Synchronization
        void waitForAllFrames(ui64 timeoutNs = UINT64_MAX);
        void createSemaphores();
        vkb::FrameSpecificObject<vk::UniqueSemaphore> imageAcquireSemaphores;
        vkb::FrameSpecificObject<vk::UniqueSemaphore> renderFinishedSemaphores;
        vkb::FrameSpecificObject<vk::UniqueFence> frameInFlightFences;

        // Default render stages
        DeferredStage defaultDeferredStage;

        // Default render passes
        RenderPass::ID defaultDeferredPass;

        struct EnabledStageType
        {
            i32 priority;
            RenderStageType::ID type;
            std::vector<RenderStage*> stages;
        };
        std::vector<EnabledStageType> enabledStages;
        std::vector<CommandCollector> commandCollectors;

        // Other things
        GlobalRenderDataDescriptor globalDataDescriptor;

        // A descriptor provider for scenes. The actual provider is
        // switched every frame.
        DescriptorProviderWrapper sceneDescriptorProvider{ SceneDescriptor::getDescLayout() };
        DescriptorProviderWrapper shadowDescriptorProvider{ ShadowDescriptor::getDescLayout() };

        vkb::DeviceLocalBuffer fullscreenQuadVertexBuffer;
    };
}
