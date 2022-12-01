#pragma once

#include <trc_util/async/ThreadPool.h>

#include "trc/base/ExclusiveQueue.h"
#include "trc/base/FrameSpecificObject.h"
#include "trc/base/PhysicalDevice.h"
#include "trc/core/Instance.h"

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
        Device& device;
        Window* window; // Must be non-const for presentImage

        // Synchronization
        void createSemaphores();
        FrameSpecific<vk::UniqueSemaphore> imageAcquireSemaphores;
        FrameSpecific<vk::UniqueSemaphore> renderFinishedSemaphores;
        FrameSpecific<vk::UniqueFence> frameInFlightFences;

        /**
         * A timeline semaphore used to signal render completion to the host.
         * I can't use the renderFinishedSemaphores for this because I have
         * to wait for the semaphore on the host, which is only possible with
         * a timeline semaphore, but vkPresentKHR does not accept timeline
         * semaphores.
         */
        FrameSpecific<vk::UniqueSemaphore> renderFinishedHostSignalSemaphores;
        FrameSpecific<ui64> renderFinishedHostSignalValue;

        async::ThreadPool threadPool;

        // Queues and command collection
        ExclusiveQueue mainRenderQueue;
        ExclusiveQueue mainPresentQueue;
    };
}
