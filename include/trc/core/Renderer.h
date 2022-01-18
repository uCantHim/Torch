#pragma once

#include <chrono>

#include <vkb/PhysicalDevice.h>
#include <vkb/FrameSpecificObject.h>
#include <vkb/ExclusiveQueue.h>
#include <vkb/event/Event.h>

namespace trc
{
    class Instance;
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
        Renderer(const Renderer&) = delete;
        Renderer(Renderer&&) = delete;
        Renderer& operator=(const Renderer&) = delete;
        Renderer& operator=(Renderer&&) = delete;

        explicit Renderer(Window& window);

        /**
         * Waits for all frames to finish rendering
         */
        ~Renderer();

        void drawFrame(const vk::ArrayProxy<const DrawConfig>& draws);

        void waitForAllFrames(std::chrono::nanoseconds timeout = std::chrono::nanoseconds::max());

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
