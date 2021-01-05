#pragma once

#include <mutex>

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
#include "DescriptorProviderWrapper.h"

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
     * @brief Static data shared among all Renderer instances
     */
    class SharedRendererData
    {
    public:
        static auto getGlobalDataDescriptorProvider() noexcept
            -> const DescriptorProviderInterface&;
        static auto getDeferredPassDescriptorProvider() noexcept
            -> const DescriptorProviderInterface&;
        static auto getShadowDescriptorProvider() noexcept
            -> const DescriptorProviderInterface&;
        static auto getSceneDescriptorProvider() noexcept
            -> const DescriptorProviderInterface&;

    protected:
        static void init(const DescriptorProviderInterface& globalDataDescriptor,
                         const DescriptorProviderInterface& deferredDescriptor);

        static auto beginRender(const DescriptorProviderInterface& globalDataDescriptor,
                                const DescriptorProviderInterface& deferredDescriptor,
                                const DescriptorProviderInterface& shadowDescriptor,
                                const DescriptorProviderInterface& sceneDescriptor)
            -> std::unique_lock<std::mutex>;

        static inline DescriptorProviderWrapper globalRenderDataProvider;
        static inline DescriptorProviderWrapper deferredDescriptorProvider;

    private:
        static inline std::mutex renderLock;
        static inline DescriptorProviderWrapper shadowDescriptorProvider;
        static inline DescriptorProviderWrapper sceneDescriptorProvider;
    };

    /**
     * @brief The heart of the Torch rendering pipeline
     *
     * Controls the rendering process based on a RenderGraph.
     */
    class Renderer : public SharedRendererData
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

        /**
         * Multiple renderers never actually render in parallel.
         */
        void drawFrame(Scene& scene, const Camera& camera);
        void drawFrame(Scene& scene, const Camera& camera, vk::Viewport viewport);

        auto getRenderGraph() noexcept -> RenderGraph&;
        auto getRenderGraph() const noexcept -> const RenderGraph&;

        auto getDeferredRenderPass() const noexcept -> const RenderPassDeferred&;

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

        vkb::DeviceLocalBuffer fullscreenQuadVertexBuffer;
    };
}
