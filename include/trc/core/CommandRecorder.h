#pragma once

#include <vector>

#include <trc_util/async/ThreadPool.h>

#include "trc/VulkanInclude.h"
#include "trc/base/FrameSpecificObject.h"

namespace trc
{
    class Device;
    class Frame;

    /**
     * @brief Records tasks' commands into command buffers
     *
     * Is responsible for multithreading the process of command recording and
     * managing the command buffers.
     */
    class CommandRecorder
    {
    public:
        CommandRecorder(const Device& device,
                        const FrameClock& frameClock,
                        async::ThreadPool* threadPool);

        CommandRecorder(const CommandRecorder&) = delete;
        auto operator=(const CommandRecorder&) -> CommandRecorder& = delete;

        CommandRecorder(CommandRecorder&&) noexcept = default;
        ~CommandRecorder() = default;

        auto operator=(CommandRecorder&&) noexcept -> CommandRecorder& = default;

        auto record(Frame& frame) -> std::vector<vk::CommandBuffer>;

    private:
        struct PerFrame
        {
            // "Use L * T pools. (L = the number of buffered frames,
            //                    T = the number of threads that record command buffers)"
            std::vector<vk::UniqueCommandPool> perThreadPools;
            std::vector<vk::UniqueCommandBuffer> perThreadCmdBuffers;
        };

        const Device* device;

        FrameSpecific<PerFrame> perFrameObjects;
        async::ThreadPool* threadPool;
    };
} // namespace trc
