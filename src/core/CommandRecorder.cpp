#include "trc/core/CommandRecorder.h"

#include <future>

#include <trc_util/algorithm/VectorTransform.h>

#include "trc/base/Device.h"
#include "trc/base/Logging.h"
#include "trc/core/DataFlow.h"
#include "trc/core/DeviceTask.h"
#include "trc/core/Frame.h"
#include "trc/core/RenderStage.h"



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



CommandRecorder::CommandRecorder(
    const Device& device,
    const FrameClock& frameClock,
    async::ThreadPool* threadPool)
    :
    device(&device),
    perFrameObjects(frameClock),
    threadPool(threadPool)
{
}

auto CommandRecorder::record(Frame& frame) -> std::vector<vk::CommandBuffer>
{
    const Device& device = *this->device;

    const auto renderGraph = frame.compileRenderGraph();
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
    const size_t numCmdBufs = renderGraph.size();
    //log::debug << "[CommandRecorder]: Recording task commands with "
    //           << numThreads << " threads.";

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
    std::vector<std::future<StageRecording>> futures;
    futures.reserve(numCmdBufs);

    // For each render stage, dispatch one thread that executes its tasks.
    ui32 threadIndex = 0;
    for (const RenderStage::ID stage : renderGraph)
    {
        vk::CommandBuffer cmdBuf = *cmdBuffers.at(threadIndex);

        // Record all tasks in the stage's task queue
        futures.emplace_back(threadPool->async(
            [&frame, stage, cmdBuf]() -> StageRecording
            {
                auto deps = std::make_shared<DependencyRegion>();
                DeviceExecutionContext ctx = frame.makeTaskExecutionContext(deps);

                cmdBuf.begin({ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
                for (auto& task : frame.iterTasks(stage))
                {
                    try {
                        task.record(cmdBuf, ctx);
                    }
                    catch (const std::exception& err)
                    {
                        log::error << "A render task in stage " << stage
                                   << " threw an error during recording."
                                   << " All commands of the task will be discarded."
                                   << " Error: " << err.what();
                        continue;
                    }
                }

                // We do NOT end the command buffer here! This is done in
                // `finalizeCmdBuffers` because additional pipeline barriers
                // need to be recorded at the end of command buffers.

                return { stage, cmdBuf, std::move(*deps) };
            }
        ));
        ++threadIndex;
    }

    // Join threads and finalize the recorded command buffers
    std::vector<StageRecording> recs;
    recs.reserve(futures.size());
    for (auto& f : futures) recs.emplace_back(f.get());

    return finalizeCmdBuffers(std::move(recs));
}

} // namespace trc
