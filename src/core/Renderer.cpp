#include "trc/core/Renderer.h"

#include <trc_util/Util.h>
#include <trc_util/Timer.h>
#include <trc_util/algorithm/VectorTransform.h>

#include "trc/core/Window.h"
#include "trc/core/DrawConfiguration.h"
#include "trc/core/RenderConfiguration.h"
#include "trc/core/FrameRenderState.h"

#include "trc/Scene.h"  // TODO: Remove reference to specialized type from core



namespace trc
{
    /**
     * Try to reserve a queue. Order:
     *  1. Reserve primary queue
     *  2. Reserve any queue
     *  3. Don't reserve, just return any queue
     */
    inline auto tryReserve(QueueManager& qm, QueueType type) -> ExclusiveQueue
    {
        if (qm.getPrimaryQueueCount(type) > 1)
        {
            return qm.reservePrimaryQueue(type);
        }
        else if (qm.getAnyQueueCount(type) > 1)
        {
            auto [queue, family] = qm.getAnyQueue(type);
            return qm.reserveQueue(queue);
        }
        else {
            return qm.getAnyQueue(type).first;
        }
    };
} // namespace trc



/////////////////////////
//      Renderer       //
/////////////////////////

trc::Renderer::Renderer(Window& _window)
    :
    instance(_window.getInstance()),
    device(_window.getDevice()),
    window(&_window),
    imageAcquireSemaphores(_window),
    renderFinishedSemaphores(_window),
    frameInFlightFences(_window),
    renderFinishedHostSignalSemaphores(_window),
    renderFinishedHostSignalValue(_window, [](ui32){ return 1; }),
    threadPool(_window.getFrameCount())
{
    createSemaphores();

    QueueManager& qm = _window.getDevice().getQueueManager();
    mainRenderQueue = tryReserve(qm, QueueType::graphics);
    mainPresentQueue = tryReserve(qm, QueueType::presentation);

    device.setDebugName(*mainRenderQueue, "main render graphics queue");
    device.setDebugName(*mainPresentQueue, "main present queue");
}

trc::Renderer::~Renderer()
{
    waitForAllFrames();

    auto& q = device.getQueueManager();
    q.freeReservedQueue(mainRenderQueue);
    q.freeReservedQueue(mainPresentQueue);
}

void trc::Renderer::drawFrame(const vk::ArrayProxy<const DrawConfig>& draws)
{
    // Wait for frame
    const auto currentFrameFence = **frameInFlightFences;
    const auto fenceResult = device->waitForFences(currentFrameFence, true, UINT64_MAX);
    if (fenceResult == vk::Result::eTimeout) {
        throw std::runtime_error("[In Renderer::drawFrame]: Timeout in waitForFences");
    }

    // Acquire image
    auto image = window->acquireImage(**imageAcquireSemaphores);

    // Record commands
    auto frameState = std::make_shared<FrameRenderState>();

    std::vector<vk::CommandBuffer> commandBuffers;
    for (const auto& draw : draws)
    {
        const RenderConfig& renderConfig = draw.renderConfig;

        // Collect commands from scene
        auto cmdBufs = renderConfig.getLayout().record(
            draw.renderConfig,
            draw.scene,
            *frameState
        );
        util::merge(commandBuffers, cmdBufs);
    }

    // Submit command buffers
    vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eComputeShader
                                       | vk::PipelineStageFlagBits::eColorAttachmentOutput;

    std::vector<vk::Semaphore> signalSemaphores{
        **renderFinishedSemaphores,
        **renderFinishedHostSignalSemaphores,
    };
    const ui64 signalValues[]{ 0, *renderFinishedHostSignalValue };
    vk::StructureChain chain{
        vk::SubmitInfo(
            **imageAcquireSemaphores,
            waitStage,
            commandBuffers,
            signalSemaphores
        ),
        vk::TimelineSemaphoreSubmitInfo(0, nullptr, 2, signalValues)
    };

    device->resetFences(currentFrameFence);
    mainRenderQueue.waitSubmit(chain.get(), currentFrameFence);

    // Dispatch asynchronous handler for when the frame has finished rendering
    threadPool.async(
        [
            this,
            sem=**renderFinishedHostSignalSemaphores,
            val=*renderFinishedHostSignalValue,
            state=std::move(frameState)
        ]() mutable {
            auto result = device->waitSemaphores(vk::SemaphoreWaitInfo({}, sem, val), UINT64_MAX);
            if (result != vk::Result::eSuccess)
            {
                throw std::runtime_error(
                    "[In Renderer::drawFrame::lambda]: vkWaitSemaphores returned a result other"
                    " than eSuccess. This should not be possible.");
            }

            state->signalRenderFinished();
        }
    );

    ++*renderFinishedHostSignalValue;

    // Present frame
    if (!window->presentImage(image, *mainPresentQueue, { **renderFinishedSemaphores })) {
        return;
    }
}

void trc::Renderer::createSemaphores()
{
    imageAcquireSemaphores = {
        *window,
        [this](ui32) { return device->createSemaphoreUnique({}); }
    };
    renderFinishedSemaphores = {
        *window,
        [this](ui32) { return device->createSemaphoreUnique({}); }
    };
    renderFinishedHostSignalSemaphores = {
        *window,
        [this](ui32) {
            vk::StructureChain chain{
                vk::SemaphoreCreateInfo(),
                vk::SemaphoreTypeCreateInfo(vk::SemaphoreType::eTimeline, 0)
            };
            return device->createSemaphoreUnique(chain.get());
        }
    };
    frameInFlightFences = {
        *window,
        [this](ui32) {
            return device->createFenceUnique(
                { vk::FenceCreateFlagBits::eSignaled }
            );
        }
    };

    for (ui32 i = 0; auto& sem : imageAcquireSemaphores) {
        device.setDebugName(*sem, "image-acquire semaphore (frame #" + std::to_string(i++) + ")");
    }
    for (ui32 i = 0; auto& sem : renderFinishedSemaphores) {
        device.setDebugName(*sem, "render-finished semaphore (frame #" + std::to_string(i++));
    }
    for (ui32 i = 0; auto& sem : renderFinishedHostSignalSemaphores) {
        device.setDebugName(*sem, "host-signal-on-render-finished semaphore (frame #" + std::to_string(i++) + ")");
    }
    for (ui32 i = 0; auto& fence : frameInFlightFences) {
        device.setDebugName(*fence, "frame-in-flight fence (frame #" + std::to_string(i++) + ")");
    }
}

void trc::Renderer::waitForAllFrames(ui64 timeoutNs)
{
    std::vector<vk::Fence> fences;
    for (auto& fence : frameInFlightFences) {
        fences.push_back(*fence);
    }
    auto result = device->waitForFences(fences, true, timeoutNs);
    if (result == vk::Result::eTimeout) {
        std::cout << "Timeout in Renderer::waitForAllFrames!\n";
    }

    device->waitIdle();
}
