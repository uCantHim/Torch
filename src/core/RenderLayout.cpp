#include "core/RenderLayout.h"

#include <atomic>
#include <ranges>
#include <future>

#include "core/Window.h"
#include "core/RenderPass.h"
#include "core/DrawConfiguration.h"
#include "core/RenderConfiguration.h"
#include "core/RenderGraph.h"
#include "core/SceneBase.h"
#include "trc_util/algorithm/VectorTransform.h"



trc::RenderLayout::RenderLayout(const Window& window, const RenderGraph& graph)
{
    using StageInfo = RenderGraph::StageInfo;
    const vkb::Device& device = window.getDevice();

    // Store stages for second dependency pass
    std::unordered_map<RenderStage::ID, std::pair<const StageInfo*, Stage*>> stageStorage;

    // Create stages
    stages.reserve(graph.size());
    for (const StageInfo& info : graph.stages)
    {
        Stage& stage = stages.emplace_back(Stage{ info.stage, info.renderPasses, {}, {} });
        stageStorage[stage.id] = { &info, &stage };
    }

    // Create dependencies
    for (auto [id, data] : stageStorage)
    {
        auto [info, stage] = data;
        for (RenderStage::ID waitStage : info->waitDependencies)
        {
            vk::Event event = *events.emplace_back(
                device->createEventUnique({ vk::EventCreateFlagBits::eDeviceOnlyKHR })
            );
            stageStorage.at(waitStage).second->signalEvents.emplace_back(event);
            stage->waitEvents.emplace_back(event);
        }
    }

    // Create command buffers
    for (ui32 i = 0; i < graph.size(); i++)
    {
        auto pool = *commandPools.emplace_back(
            device->createCommandPoolUnique({
                vk::CommandPoolCreateFlagBits::eResetCommandBuffer
                | vk::CommandPoolCreateFlagBits::eTransient
            })
        );
        auto& swapchain = window.getSwapchain();
        commandBuffers.emplace_back(
            swapchain,
            device->allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo(
                pool, vk::CommandBufferLevel::ePrimary, swapchain.getFrameCount()
            ))
        );
    }
}

auto trc::RenderLayout::record(
    RenderConfig& config,
    SceneBase& scene,
    FrameRenderState& state)
    -> std::vector<vk::CommandBuffer>
{
    std::vector<std::future<Maybe<vk::CommandBuffer>>> futures;

    // Dispatch parallel collector threads
    for (std::atomic<ui32> index = 0; const Stage& stage : stages)
    {
        vk::CommandBuffer cmdBuf = **commandBuffers.at(index++);
        futures.emplace_back(threadPool->async([&, cmdBuf] {
            return recordStage(cmdBuf, config, scene, state, stage);
        }));
    }

    // Insert recorded command buffers *in order*
    std::vector<vk::CommandBuffer> result;
    for (auto& f : futures) {
        f.get() >> [&](auto cmdBuf) { result.emplace_back(cmdBuf); };
    }

    return result;
}

auto trc::RenderLayout::recordStage(
    vk::CommandBuffer cmdBuf,
    RenderConfig& config,
    SceneBase& scene,
    FrameRenderState& frameState,
    const Stage& stage)
    -> Maybe<vk::CommandBuffer>
{
    const auto renderPasses = util::merged(stage.renderPasses,
                                           scene.getDynamicRenderPasses(stage.id));

    // Only signal outgoing events if no renderpasses are specified
    if (renderPasses.empty())
    {
        // Don't record anything if not necessary
        if (stage.signalEvents.empty()) return {};

        cmdBuf.reset({});
        cmdBuf.begin({ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
        for (auto event : stage.signalEvents) {
            cmdBuf.setEvent(event, vk::PipelineStageFlagBits::eAllCommands);
        }
        cmdBuf.end();

        return cmdBuf;
    }

    cmdBuf.reset({});
    cmdBuf.begin({ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });

    // Wait for event dependencies
    if (!stage.waitEvents.empty())
    {
        cmdBuf.waitEvents(
            stage.waitEvents,
            vk::PipelineStageFlagBits::eAllCommands,
            vk::PipelineStageFlagBits::eAllCommands,
            {}, {}, {}
        );
    }
    for (auto event : stage.waitEvents) {
        cmdBuf.resetEvent(event, vk::PipelineStageFlagBits::eAllCommands);
    }

    // Record render passes
    for (auto renderPass : renderPasses)
    {
        assert(renderPass != nullptr);

        renderPass->begin(cmdBuf, vk::SubpassContents::eInline, frameState);

        // Record all commands
        const ui32 subPassCount = renderPass->getNumSubPasses();
        for (ui32 subPass = 0; subPass < subPassCount; subPass++)
        {
            for (auto pipeline : scene.iterPipelines(stage.id, SubPass::ID(subPass)))
            {
                // Bind the current pipeline
                auto& p = config.getPipeline(pipeline);
                p.bind(cmdBuf, config);

                // Record commands for all objects with this pipeline
                scene.invokeDrawFunctions(
                    stage.id, *renderPass, SubPass::ID(subPass),
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

    // Signal outgoing dependencies
    for (auto event : stage.signalEvents) {
        cmdBuf.setEvent(event, vk::PipelineStageFlagBits::eAllCommands);
    }

    cmdBuf.end();

    return cmdBuf;
}

void trc::RenderLayout::addPass(RenderStage::ID stage, RenderPass& newPass)
{
    Stage* s = getStage(stage);
    if (s == nullptr)
    {
        throw std::invalid_argument("[In RenderLayout::addPass]: Stage " + std::to_string(stage)
                                    + " is not present in the layout.");

    }

    s->renderPasses.emplace_back(&newPass);
}

void trc::RenderLayout::removePass(RenderStage::ID stage, RenderPass& pass)
{
    Stage* s = getStage(stage);
    if (s == nullptr)
    {
        throw std::invalid_argument("[In RenderLayout::removePass]: Stage " + std::to_string(stage)
                                    + " is not present in the layout.");

    }

    auto it = std::ranges::find(s->renderPasses, &pass);
    if (it != s->renderPasses.end()) {
        s->renderPasses.erase(it);
    }
}

auto trc::RenderLayout::getStage(RenderStage::ID stage) -> Stage*
{
    auto it = std::ranges::find_if(stages, [stage](auto& a) { return a.id == stage; });
    if (it != stages.end()) {
        return &*it;
    }
    return nullptr;
}
