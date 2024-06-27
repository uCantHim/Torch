#include "trc/core/CommandRecorder.h"

#include <future>

#include <trc_util/algorithm/VectorTransform.h>

#include "trc/base/Logging.h"
#include "trc/core/Frame.h"
#include "trc/core/RenderConfiguration.h"
#include "trc/core/Task.h"



namespace trc
{

struct StageRecording
{
    RenderStage::ID stage;
    vk::CommandBuffer cmdBuf;
    DependencyRegion resourceDependencies;
};

auto finalizeCmdBuffers(std::vector<StageRecording> recordings)
    -> std::vector<vk::CommandBuffer>
{
    std::vector<vk::CommandBuffer> result;
    for (size_t i = 0; i < recordings.size(); ++i)
    {
        auto cmdBuf = recordings[i].cmdBuf;
        if ((i + 1) < recordings.size())
        {
            auto [buf, img] = DependencyRegion::genBarriers(
                recordings[i].resourceDependencies,
                recordings[i + 1].resourceDependencies
            );
            if (!buf.empty() || !img.empty()) {
                cmdBuf.pipelineBarrier2(vk::DependencyInfo{ {}, {}, buf, img });
            }
        }

        cmdBuf.end();
        result.emplace_back(cmdBuf);
    }

    return result;
}

} // namespace trc



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
    const size_t numCmdBufs = [&]{
        size_t res{ 0 };
        for (const auto& vp : frame.getViewports()) {
            for (const auto& stage : vp->taskQueue.tasks) {
                res += stage.size();
            }
        }
        return res;
    }();
    //log::debug << "[CommandRecorder]: Recording task commands with "
    //           << numThreads << " threads.";

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

    // Possibly create new pools if more are required
    while (pools.size() < numCmdBufs)
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

    // Each viewport has its own list of command buffer recordings (one command
    // buffer for each render stage) that define resource dependencies among
    // each other.
    std::vector<std::vector<std::future<StageRecording>>> futuresPerVp;
    futuresPerVp.reserve(frame.getViewports().size());

    // Dispatch command buffer recorder threads.
    ui32 threadIndex = 0;
    for (auto& vp : frame.getViewports())
    {
        auto& renderGraph = vp->config->getRenderGraph();
        auto& futures = futuresPerVp.emplace_back();

        // For each render stage, dispatch one thread that executes its tasks.
        for (const RenderStage::ID stage : renderGraph)
        {
            auto tasks = vp->taskQueue.tasks.try_get(stage);
            if (!tasks || tasks->empty()) {
                continue;
            }

            vk::CommandBuffer cmdBuf = *cmdBuffers.at(threadIndex);

            // Record all tasks in the stage's task queue
            futures.emplace_back(threadPool->async(
                [&device, &frame, tasks, &vp, cmdBuf, stage]() -> StageRecording
                {
                    TaskEnvironment env{ {}, device, &frame, vp->resources, vp->scene };

                    cmdBuf.begin({ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
                    for (auto& task : *tasks) {
                        task->record(cmdBuf, env);
                    }
                    // We do NOT end the command buffer here! This is done in
                    // `finalizeCmdBuffers` because additional pipeline barriers
                    // need to be recorded at the end of command buffers.
                    // cmdBuf.end();

                    return { stage, cmdBuf, env.getDependencyRegion() };
                }
            ));
            ++threadIndex;
        }
    }

    // Join threads and finalize the recorded command buffers
    std::vector<vk::CommandBuffer> result;
    for (auto& vp : futuresPerVp)
    {
        std::vector<StageRecording> recs;
        recs.reserve(vp.size());
        for (auto& f : vp) recs.emplace_back(f.get());

        auto cmdBufs = finalizeCmdBuffers(std::move(recs));
        util::merge(result, cmdBufs);
    }

    return result;
}
