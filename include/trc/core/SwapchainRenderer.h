#pragma once

#include "trc/Types.h"
#include "trc/base/FrameSpecificObject.h"
#include "trc/core/Renderer.h"

namespace trc
{
    class Device;
    class Swapchain;
    class Frame;

    /**
     * @brief Integrates rendering with swapchain image acquisition
     *
     * A wrapper around `Renderer` that manages additional synchronization
     * primitives required to tie command execution together with acquisition
     * and submission of swapchain images.
     */
    class SwapchainRenderer
    {
    public:
        SwapchainRenderer(Device& _device, Swapchain& swapchain);
        ~SwapchainRenderer() noexcept;

        /**
         * @brief Render a frame
         *
         * Synchronizes rendering (command execution) with the acquisition and
         * submittion of a swapchain image.
         */
        void renderFrame(u_ptr<Frame> _frame);

        void waitForAllFrames(ui64 timeoutNanoseconds = UINT64_MAX);

    private:
        Device* device;
        Swapchain* swapchain;
        Renderer renderer;

        // Synchronization
        FrameSpecific<vk::UniqueSemaphore> imageAcquireSemaphores;
        FrameSpecific<vk::UniqueSemaphore> renderFinishedSemaphores;

        // Queues and command collection
        ExclusiveQueue mainRenderQueue;
        ExclusiveQueue mainPresentQueue;
    };
} // namespace trc
