#include "RenderEngine.h"

#include <algorithm>
#include <cassert>
#include <iostream>

#include <glm/glm.hpp>

#include "RenderEnvironment.h"
#include "VulkanDrawable.h"
#include "Scene.h"



// -------------------------------- //
//        CommandCollector class        //
// -------------------------------- //


CommandCollector::CommandCollector()
    :
    primaryCommandBuffer([](uint32_t) {
        return vkb::VulkanBase::getDevice().createGraphicsCommandBuffer();
    })
{
    std::thread(std::bind(&CommandCollector::run, this)).detach();
}


auto CommandCollector::collect(
    const Renderpass& renderpass,
    draw_iter begin,
    draw_iter end) -> std::future<vk::CommandBuffer>
{
    auto newCommand = CollectCommand{
        &renderpass,
        begin,
        end,
        {} // result promise
    };
    auto future = newCommand.result.get_future();
    commandQueue.push(std::move(newCommand));
    return future;
}


auto CommandCollector::terminate() noexcept -> std::future<bool>
{
    shouldTerminate = true;
    return isTerminated.get_future();
}


void CommandCollector::run()
{
    while (!shouldTerminate)
    {
        if (commandQueue.empty())
            continue;

        auto& command = commandQueue.front();
        auto& [renderpass, begin, end, promisedResult] = command;

        vk::CommandBufferInheritanceInfo inheritanceInfo {
            **renderpass,
            0, // subpass
            {}, // framebuffer
            VK_FALSE,
            vk::QueryControlFlags(),
            vk::QueryPipelineStatisticFlags()
        };

        // Begin recording
        auto cmdBuf = **primaryCommandBuffer;
        cmdBuf.reset({});
        cmdBuf.begin(vk::CommandBufferBeginInfo{
            vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
            nullptr // inheritance info, ignored for primary buffers
        });
        renderpass->beginCommandBuffer(cmdBuf);

        // Record all subpasses
        for (size_t subpassIndex = 0; subpassIndex < renderpass->getSubpassCount(); subpassIndex++)
        {
            inheritanceInfo.subpass = subpassIndex;

            std::vector<vk::CommandBuffer> secondaryBuffers;
            secondaryBuffers.reserve(end - begin);
            for (auto curr = begin; curr < end; curr++)
            {
                secondaryBuffers.push_back((*curr)->recordCommandBuffer(subpassIndex, inheritanceInfo));
            }

            if (!secondaryBuffers.empty())
                cmdBuf.executeCommands(secondaryBuffers);
            if (subpassIndex < renderpass->getSubpassCount() - 1)
                cmdBuf.nextSubpass(vk::SubpassContents::eSecondaryCommandBuffers);
        }

        // End recording
        cmdBuf.endRenderPass();
        cmdBuf.end();

        // Publish result
        promisedResult.set_value(cmdBuf);
        commandQueue.pop();
    }
    isTerminated.set_value(true);
}



// ---------------------------- //
//        RenderEngine class        //
// ---------------------------- //

RenderEngine::RenderEngine(const Renderpass& renderpass)
    :
    renderpass(renderpass),
    collectors(COMMAND_COLLECTOR_COUNT)
{
}


RenderEngine::~RenderEngine()
{
    // Stop all collector threads. Wait for them to terminate before exiting.
    for (auto& collector : collectors)
    {
        collector.terminate().get();
    }
}


auto RenderEngine::recordScene(Scene& scene) -> std::vector<vk::CommandBuffer>
{
    std::vector<std::future<vk::CommandBuffer>> futureResults;
    futureResults.reserve(COMMAND_COLLECTOR_COUNT);

    futureResults.push_back(collectors[0].collect(
        renderpass,
        scene.drawables.begin(),
        scene.drawables.end()
    ));

    std::vector<vk::CommandBuffer> result;
    result.reserve(COMMAND_COLLECTOR_COUNT);
    for (size_t i = 0; i < futureResults.size(); i++)
    {
        try {
            result.push_back(futureResults[i].get());
        }
        catch (const std::exception& err) {
            if constexpr (vkb::enableVerboseLogging) {
                std::cout << "Future exception in command collect result: " << err.what() << "\n";
            }
            throw err;
        }
    }

    return result;
}
