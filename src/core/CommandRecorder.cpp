#include "trc/core/CommandRecorder.h"

#include <future>

#include "trc/base/Logging.h"
#include "trc/core/Frame.h"
#include "trc/core/RenderConfiguration.h"
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
            for (const auto& stage : vp->taskQueue.tasks) {
                res += stage.size();
            }
        }
        return res;
    }();

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

    struct StageRecording
    {
        RenderStage::ID stage;
        vk::CommandBuffer cmdBuf;
        DependencyRegion resourceDependencies;
    };

    // Dispatch command buffer recorder threads.
    std::vector<std::future<StageRecording>> futures;
    futures.reserve(numThreads);

    // Idea:
    //
    // std::vector<std::pair<RenderStage::ID, std::future<vk::CommandBuffer>>>;
    //
    // Then insert all barriers from each stage to its depending stages at the end
    // if the respective command buffer according to the stage's dependency region.
    //
    // This requires a real render stage graph that defines a complex ordering
    // of stages with respect to each other.

    ui32 threadIndex = 0;
    for (auto& vp : frame.getViewports())
    {
        auto& renderGraph = vp->config->getRenderGraph();

        // For each render stage, dispatch one thread that executes its tasks.
        for (const RenderStage::ID stage : renderGraph)
        {
            auto& tasks = vp->taskQueue.tasks.get(stage);
            if (tasks.empty()) {
                break;
            }

            vk::CommandBuffer cmdBuf = *cmdBuffers.at(threadIndex);

            // Record all tasks in the stage's task queue
            futures.emplace_back(threadPool->async(
                [&device, &frame, &tasks, &vp, cmdBuf, stage]() -> StageRecording
                {
                    TaskEnvironment env{ {}, device, &frame, vp->resources, vp->scene };

                    cmdBuf.begin({ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
                    for (auto& task : tasks) {
                        task->record(cmdBuf, env);
                    }
                    cmdBuf.end();

                    return { stage, cmdBuf, env };
                }
            ));
            ++threadIndex;
        }
    }

    // Join threads.
    // Insert recorded command buffers *in order*.
    std::vector<vk::CommandBuffer> result;
    std::unordered_map<RenderStage::ID, StageRecording> recordings;
    for (auto& f : futures)
    {
        auto rec = f.get();
        recordings.try_emplace(rec.stage, rec);
        result.emplace_back(rec.cmdBuf);
    }

    return result;
}
