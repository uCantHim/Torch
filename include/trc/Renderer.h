#pragma once

#include <vkb/VulkanBase.h>
#include <vkb/Buffer.h>
#include <vkb/FrameSpecificObject.h>
#include <vkb/event/Event.h>

#include "TorchResources.h"
#include "RenderGraph.h"
#include "RenderPassDeferred.h"
#include "RenderDataDescriptor.h"
#include "SceneDescriptor.h"
#include "LightRegistry.h" // For shadow descriptor
#include "CommandCollector.h"

namespace trc
{
    class Renderer;
    class Scene;

    struct RendererCreateInfo
    {
        vkb::Swapchain* swapchain;
        ui32 maxTransparentFragsPerPixel{ 3 };
    };

    /**
     * @brief The heart of the Torch rendering pipeline
     */
    class Renderer
    {
    public:
        explicit Renderer(RendererCreateInfo info = {});

        /**
         * Waits for all frames to finish rendering
         */
        ~Renderer();

        Renderer(const Renderer&) = delete;
        Renderer(Renderer&&) = delete;
        Renderer& operator=(const Renderer&) = delete;
        Renderer& operator=(Renderer&&) = delete;

        void drawFrame(Scene& scene, const Camera& camera);

        auto getRenderGraph() noexcept -> RenderGraph&;
        auto getRenderGraph() const noexcept -> const RenderGraph&;

        auto getDeferredRenderPass() const noexcept -> const RenderPassDeferred&;

        auto getGlobalDataDescriptor() const noexcept -> const GlobalRenderDataDescriptor&;
        auto getGlobalDataDescriptorProvider() const noexcept -> const DescriptorProviderInterface&;
        auto getSceneDescriptorProvider() const noexcept -> const DescriptorProviderInterface&;
        auto getShadowDescriptorProvider() const noexcept -> const DescriptorProviderInterface&;

        /**
         * This function basically just calls glm::unProject.
         *
         * @param const Camera& camera Reconstructing the mouse coursor's
         *        world position requires the view- and projection matrices
         *        that were used for the scene that the cursor is in.
         *
         * @return vec3 Position of the mouse cursor in the world.
         */
        auto getMouseWorldPos(const Camera& camera) -> vec3;

        /**
         * @brief Calculate the mouse cursor position in the world at a
         *        specific depth.
         *
         * @param const Camera& camera Reconstructing the mouse coursor's
         *        world position requires the view- and projection matrices
         *        that were used for the scene that the cursor is in.
         * @param float         depth  A depth value in the range [0, 1].
         *
         * @return vec3 Position of the mouse cursor in the world.
         */
        auto getMouseWorldPosAtDepth(const Camera& camera, float depth) -> vec3;

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

        // Default render passes
        RenderPass::ID defaultDeferredPass;

        RenderGraph renderGraph;
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
