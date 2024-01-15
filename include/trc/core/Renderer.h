#pragma once

#include <trc_util/async/ThreadPool.h>

#include "trc/base/Device.h"
#include "trc/base/ExclusiveQueue.h"
#include "trc/base/FrameSpecificObject.h"
#include "trc/core/CommandRecorder.h"

namespace trc
{
    class Frame;

    /**
     * @brief Records commands from `Frame` objects and submits them to a queue.
     *
     * Handles synchronization from device commands to host as well as among
     * frames.
     */
    class Renderer
    {
    public:
        Renderer(const Device& device, const FrameClock& clock);

        /**
         * Waits for all frames to finish rendering
         */
        ~Renderer() noexcept;

        Renderer(const Renderer&) = delete;
        Renderer(Renderer&&) = delete;
        Renderer& operator=(const Renderer&) = delete;
        Renderer& operator=(Renderer&&) = delete;

        /**
         * @brief Execute a frame's commands on the device.
         *
         * Records all device commands defined by tasks in a frame, then submits
         * these commands to a queue for execution on the device.
         *
         * Dispatches a thread that waits for this specific frame's commands
         * to be executed, then calls its registered `onRenderFinish` callbacks.
         *
         * @param s_ptr<Frame>    frame The frame to draw.
         * @param ExclusiveQueue& queue The queue to which recorded device
         *                              commands will be submitted.
         * @param vk::Semaphore waitSemaphore   A semaphore that must be signalled
         *                                      before any commands are executed
         *                                      on the device. May be `VK_NULL_HANDLE`.
         * @param vk::Semaphore signalSemaphore A semaphore to be signalled once
         *                                      all device commands have finished
         *                                      executing. May be `VK_NULL_HANDLE`.
         */
        void renderFrame(s_ptr<Frame> frame,
                         ExclusiveQueue& queue,
                         vk::Semaphore waitSemaphore,
                         vk::Semaphore signalSemaphore);

        /**
         * @brief Wait for all frames that are being rendered to finish.
         *
         * Multiple frames may be 'in flight' at the same time, that is, queued
         * for execution on the device. This function waits for all currently
         * queued commands to be fully executed, then returns.
         *
         * @param ui64 timeoutNs A maximum amount of time to wait, in
         *                       nanoseconds. Specify the maximum value of
         *                       `UINT64_MAX` to set no timeout. This function
         *                       returns `false` if the timeout is exceeded
         *                       before all frames have finished executing.
         *
         * @return bool `true` if waiting succeeded within the specified timeout,
         *              `false` otherwise.
         */
        bool waitForAllFrames(ui64 timeoutNs = UINT64_MAX);

    private:
        struct RenderFinishedHandler
        {
            const Device& device;
            vk::Semaphore waitSem;
            ui64 waitVal;
            s_ptr<Frame> frame;

            void operator()();
        };

        const Device& device;

        async::ThreadPool threadPool;
        CommandRecorder cmdRecorder;

        // Synchronization
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
    };
}
