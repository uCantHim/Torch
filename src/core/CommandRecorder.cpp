#include "trc/core/CommandRecorder.h"

#include <algorithm>
#include <future>
#include <numeric>
#include <ranges>
#include <stdexcept>

#include "trc/core/Window.h"
#include "trc/core/RenderPass.h"
#include "trc/core/DrawConfiguration.h"
#include "trc/core/RenderConfiguration.h"
#include "trc/core/RenderGraph.h"
#include "trc/RasterSceneBase.h"
#include "trc_util/algorithm/VectorTransform.h"



trc::CommandRecorder::CommandRecorder(const Device& device, const FrameClock& frameClock)
    :
    device(&device),
    perFrameObjects(frameClock)
{
}

auto trc::CommandRecorder::record(
    const vk::ArrayProxy<const DrawConfig>& draws,
    FrameRenderState& state)
    -> std::vector<vk::CommandBuffer>
{
    const size_t numThreads = [&]{
        size_t res{ 0 };
        for (const auto& draw : draws) res += draw.renderConfig.getRenderGraph().size();
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

    // Dispatch command buffer recorder threads.
    std::vector<std::future<std::optional<vk::CommandBuffer>>> futures;
    futures.reserve(numThreads);

    for (ui32 threadIndex = 0; const auto& draw : draws)
    {
        for (const auto& stage : draw.renderConfig.getRenderGraph().getStages())
        {
            vk::CommandBuffer cmdBuf = *cmdBuffers.at(threadIndex);
            futures.emplace_back(threadPool->async([&, cmdBuf] {
                return recordStage(cmdBuf, draw.renderConfig, draw.scene, state, stage);
            }));

            ++threadIndex;
        }
    }

    // Join threads.
    // Insert recorded command buffers *in order*.
    std::vector<vk::CommandBuffer> result;
    for (auto& f : futures)
    {
        if (auto cmdBuf = f.get()) {
            result.emplace_back(*cmdBuf);
        };
    }

    return result;
}

auto trc::CommandRecorder::recordStage(
    vk::CommandBuffer cmdBuf,
    RenderConfig& config,
    const RasterSceneBase& scene,
    FrameRenderState& frameState,
    const RenderGraph::StageInfo& stage) const
    -> std::optional<vk::CommandBuffer>
{
    const auto renderStageId = stage.stage;
    const auto renderPasses = util::merged(stage.renderPasses,
                                           scene.getDynamicRenderPasses(renderStageId));

    // Don't record anything if no renderpasses are specified.
    if (renderPasses.empty()) {
        return {};
    }

    cmdBuf.begin({ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });

    // Record render passes
    for (auto renderPass : renderPasses)
    {
        assert(renderPass != nullptr);

        renderPass->begin(cmdBuf, vk::SubpassContents::eInline, frameState);

        // Record all commands
        const ui32 subPassCount = renderPass->getNumSubPasses();
        for (ui32 subPass = 0; subPass < subPassCount; subPass++)
        {
            for (auto pipeline : scene.iterPipelines(renderStageId, SubPass::ID(subPass)))
            {
                // Bind the current pipeline
                auto& p = config.getPipeline(pipeline);
                p.bind(cmdBuf, config);

                // Record commands for all objects with this pipeline
                scene.invokeDrawFunctions(
                    renderStageId, *renderPass, SubPass::ID(subPass),
                    pipeline, p,
                    cmdBuf
                );
            }

            if (subPass < subPassCount - 1) {
                cmdBuf.nextSubpass(vk::SubpassContents::eInline);
            }
        }

        renderPass->end(cmdBuf);
    }

    cmdBuf.end();

    return cmdBuf;
}
