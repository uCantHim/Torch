#include "core/RenderLayout.h"

#include <atomic>
#include <ranges>
#include <future>

#include "core/Window.h"
#include "core/RenderPass.h"
#include "core/DrawConfiguration.h"
#include "core/RenderGraph.h"
#include "util/algorithm/VectorTransform.h"



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
        Stage& stage = stages.emplace_back(info.stage, info.renderPasses);
        stageStorage[stage.id] = { &info, &stage };
    }

    // Create dependencies
    for (auto [id, data] : stageStorage)
    {
        auto [info, stage] = data;
        for (RenderStage::ID waitStage : info->waitDependencies)
        {
            vk::Event event = *events.emplace_back(device->createEventUnique({}));
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

auto trc::RenderLayout::record(const DrawConfig& draw) -> std::vector<vk::CommandBuffer>
{
    std::vector<std::future<void>> futures;
    std::vector<vk::CommandBuffer> result(stages.size());

    for (std::atomic<ui32> index = 0; const Stage& stage : stages)
    {
        ui32 i = index++;
        vk::CommandBuffer cmdBuf = **commandBuffers.at(i);
        result[i] = cmdBuf;

        futures.emplace_back(threadPool.async([&, cmdBuf] {
            recordStage(cmdBuf, draw, stage);
        }));
    }

    for (auto& f : futures) {
        f.wait();
    }

    return result;
}

void trc::RenderLayout::recordStage(
    vk::CommandBuffer cmdBuf,
    const DrawConfig& draw,
    const Stage& stage)
{
    SceneBase& scene = *draw.scene;
    const auto& [viewport, scissor] = draw.renderArea;
    const auto renderPasses = util::merged(stage.renderPasses,
                                           scene.getDynamicRenderPasses(stage.id));
    if (renderPasses.empty()) return;

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

        renderPass->begin(cmdBuf, vk::SubpassContents::eInline);

        // Record all commands
        const ui32 subPassCount = renderPass->getNumSubPasses();
        for (ui32 subPass = 0; subPass < subPassCount; subPass++)
        {
            for (auto pipeline : scene.getPipelines(stage.id, SubPass::ID(subPass)))
            {
                // Bind the current pipeline
                auto& p = draw.renderConfig->getPipeline(pipeline);
                p.bind(cmdBuf);
                p.bindStaticDescriptorSets(cmdBuf);
                p.bindDefaultPushConstantValues(cmdBuf);

                cmdBuf.setViewport(0, viewport);
                cmdBuf.setScissor(0, scissor);

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
