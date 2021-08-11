#pragma once

#include <mutex>

#include <vkb/basics/Device.h>
#include <vkb/FrameSpecificObject.h>
#include <vkb/event/Event.h>

#include "CommandCollector.h"

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

        /**
         * Multiple renderers never actually render in parallel.
         */
        void drawFrame(const DrawConfig& draw);

    private:
        const Instance& instance;
        const vkb::Device& device;
        Window* window; // Must be non-const for presentImage

        // Synchronization
        void waitForAllFrames(ui64 timeoutNs = UINT64_MAX);
        void createSemaphores();
        vkb::FrameSpecificObject<vk::UniqueSemaphore> imageAcquireSemaphores;
        vkb::FrameSpecificObject<vk::UniqueSemaphore> renderFinishedSemaphores;
        vkb::FrameSpecificObject<vk::UniqueFence> frameInFlightFences;

        CommandCollector commandCollector;
    };
}
