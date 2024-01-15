#include "trc/core/CommandRecorder.h"

#include <future>

#include "trc/core/Frame.h"
#include "trc/core/Task.h"



trc::CommandRecorder::CommandRecorder(
    const Device& device,
    const FrameClock& frameClock,
    async::ThreadPool* threadPool)
    :
    device(&device),
    perFrameObjects(frameClock),
    threadPool(threadPool)
{
}

auto trc::CommandRecorder::record(Frame& frame) -> std::vector<vk::CommandBuffer>
{
    const size_t numThreads = [&]{
        size_t res{ 0 };
        for (const auto& vp : frame.getViewports()) {
            for (const auto& stage : vp.taskQueue.tasks) {
                res += stage.size();
            }
        }
        return res;
    }();
    std::cout << "[CommandRecorder]: Recording task commands with " << numThreads << " threads.\n";

    const Device& device = *this->device;

    auto& resources = perFrameObjects.get();
    auto& pools = resources.perThreadPools;
    auto& cmdBuffers = resources.perThreadCmdBuffers;

    // Reset existing command pools. This also resets existing command buffers.
    //
    // "Call vkResetCommandPool before reusing it in another frame. Otherwise,
    // the pool will keep on growing until you run out of memory."
    //
    //   source: https://developer.nvidia.com/blog/vulkan-dos-donts/
    for (auto& pool : pools) {
        device->resetCommandPool(*pool, {});
    }

    // Possibly create new pools if more threads are required
    while (pools.size() < numThreads)
    {
        auto pool = *pools.emplace_back(device->createCommandPoolUnique(
            vk::CommandPoolCreateInfo({}, {})
        ));

        // Create a command buffer for the thread
        cmdBuffers.emplace_back(
            std::move(device->allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo(
                pool, vk::CommandBufferLevel::ePrimary, 1
            ))[0])
        );
    }

    // Dispatch command buffer recorder threads.
    std::vector<std::future<vk::CommandBuffer>> futures;
    futures.reserve(numThreads);

    for (ui32 threadIndex = 0; auto& vp : frame.getViewports())
    {
        for (auto [stage, tasks] : vp.taskQueue.tasks.items())
        {
            if (tasks.empty()) {
                continue;
            }

            vk::CommandBuffer cmdBuf = *cmdBuffers.at(threadIndex);

            // Record all tasks in the stage's task queue
            futures.emplace_back(threadPool->async([&, cmdBuf]() -> vk::CommandBuffer {
                cmdBuf.begin({ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
                for (auto& task : tasks)
                {
                    TaskEnvironment env{ &frame, stage, vp.viewport, vp.scene };
                    task->record(cmdBuf, env);
                }
                cmdBuf.end();

                return cmdBuf;
            }));
            ++threadIndex;
        }
    }

    // Join threads.
    // Insert recorded command buffers *in order*.
    std::vector<vk::CommandBuffer> result;
    for (auto& f : futures) {
        result.emplace_back(f.get());
    }

    return result;
}
