#pragma once

#include <vkb/PhysicalDevice.h>
#include <vkb/FrameSpecificObject.h>
#include <vkb/ExclusiveQueue.h>
#include <vkb/event/Event.h>
#include <trc_util/async/ThreadPool.h>

#include "Instance.h"

namespace trc
{
    class Window;
    struct DrawConfig;
    class FrameRenderState;

    /**
     * @brief The heart of the Torch rendering pipeline
     *
     * Controls the rendering process based on a RenderGraph.
     */
    class Renderer
    {
    public:
        explicit Renderer(Window& window);

        /**
         * Waits for all frames to finish rendering
         */
        ~Renderer();

        Renderer(const Renderer&) = delete;
        Renderer(Renderer&&) = delete;
        Renderer& operator=(const Renderer&) = delete;
        Renderer& operator=(Renderer&&) = delete;

        void drawFrame(const vk::ArrayProxy<const DrawConfig>& draws);

        void waitForAllFrames(ui64 timeoutNs = UINT64_MAX);

    private:
        struct RenderFinishedHandler
        {
            RenderFinishedHandler(vk::Semaphore sem, ui64 waitValue, u_ptr<FrameRenderState> state);

            void operator()();

            vk::Semaphore sem;
            ui64 waitValue;
            u_ptr<FrameRenderState> state;
        };

        const Instance& instance;
        vkb::Device& device;
        Window* window; // Must be non-const for presentImage

        vkb::UniqueListenerId<vkb::PreSwapchainRecreateEvent> swapchainRecreateListener;

        // Synchronization
        void createSemaphores();
        vkb::FrameSpecific<vk::UniqueSemaphore> imageAcquireSemaphores;
        vkb::FrameSpecific<vk::UniqueSemaphore> renderFinishedSemaphores;
        vkb::FrameSpecific<vk::UniqueFence> frameInFlightFences;

        /**
         * A timeline semaphore used to signal render completion to the host.
         * I can't use the renderFinishedSemaphores for this because I have
         * to wait for the semaphore on the host, which is only possible with
         * a timeline semaphore, but vkPresentKHR does not accept timeline
         * semaphores.
         */
        vkb::FrameSpecific<vk::UniqueSemaphore> renderFinishedHostSignalSemaphores;
        vkb::FrameSpecific<ui64> renderFinishedHostSignalValue;

        async::ThreadPool threadPool;

        // Queues and command collection
        vkb::ExclusiveQueue mainRenderQueue;
        vkb::QueueFamilyIndex mainRenderQueueFamily;
        vkb::ExclusiveQueue mainPresentQueue;
        vkb::QueueFamilyIndex mainPresentQueueFamily;
    };
}
