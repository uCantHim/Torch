#pragma once

#include <mutex>

#include <vkb/PhysicalDevice.h>
#include <vkb/FrameSpecificObject.h>
#include <vkb/ExclusiveQueue.h>
#include <vkb/event/Event.h>

#include "Instance.h"

namespace trc
{
    class Window;
    struct DrawConfig;

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
        const Instance& instance;
        vkb::Device& device;
        Window* window; // Must be non-const for presentImage

        vkb::UniqueListenerId<vkb::PreSwapchainRecreateEvent> swapchainRecreateListener;

        // Synchronization
        void createSemaphores();
        vkb::FrameSpecific<vk::UniqueSemaphore> imageAcquireSemaphores;
        vkb::FrameSpecific<vk::UniqueSemaphore> renderFinishedSemaphores;
        vkb::FrameSpecific<vk::UniqueFence> frameInFlightFences;

        // Queues and command collection
        vkb::ExclusiveQueue mainRenderQueue;
        vkb::QueueFamilyIndex mainRenderQueueFamily;
        vkb::ExclusiveQueue mainPresentQueue;
        vkb::QueueFamilyIndex mainPresentQueueFamily;
    };
}
