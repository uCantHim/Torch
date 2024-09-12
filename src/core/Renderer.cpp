#include "trc/core/Renderer.h"

#include <vector>

#include "trc/base/Logging.h"
#include "trc/base/Swapchain.h"
#include "trc/core/Frame.h"



namespace trc
{
    /**
     * Try to reserve a queue. Order:
     *  1. Reserve primary queue
     *  2. Reserve any queue
     *  3. Don't reserve, just return any queue
     */
    static inline auto tryReserve(QueueManager& qm, QueueType type) -> ExclusiveQueue
    {
        if (qm.getPrimaryQueueCount(type) > 1) {
            return qm.reservePrimaryQueue(type);
        }
        if (qm.getAnyQueueCount(type) > 1)
        {
            auto [queue, family] = qm.getAnyQueue(type);
            return qm.reserveQueue(queue);
        }
        return qm.getAnyQueue(type).first;
    };
} // namespace trc



/////////////////////////
//      Renderer       //
/////////////////////////

trc::Renderer::Renderer(Device& _device, const FrameClock& clock)
    :
    device(_device),
    threadPool(std::max(1u, std::thread::hardware_concurrency() - 1u)),
    cmdRecorder(_device, clock, &threadPool),
    frameInFlightFences(clock),
    renderFinishedHostSignalSemaphores(clock),
    renderFinishedHostSignalValue(clock, [](ui32){ return 1; }),
    swapchainImageAcquireSemaphores(clock),
    renderFinishedSemaphores(clock)
{
    // Create synchronization primitives
    renderFinishedHostSignalSemaphores = {
        clock,
        [this](ui32) {
            vk::StructureChain chain{
                vk::SemaphoreCreateInfo(),
                vk::SemaphoreTypeCreateInfo(vk::SemaphoreType::eTimeline, 0)
            };
            return device->createSemaphoreUnique(chain.get());
        }
    };
    frameInFlightFences = {
        clock,
        [this](ui32) {
            return device->createFenceUnique(
                { vk::FenceCreateFlagBits::eSignaled }
            );
        }
    };
    swapchainImageAcquireSemaphores = {
        clock,
        [&](ui32) { return device->createSemaphoreUnique({}); }
    };
    renderFinishedSemaphores = {
        clock,
        [&](ui32) { return device->createSemaphoreUnique({}); }
    };

    for (ui32 i = 0; auto& sem : renderFinishedHostSignalSemaphores) {
        device.setDebugName(*sem, "host-signal-on-render-finished semaphore (frame #" + std::to_string(i++) + ")");
    }
    for (ui32 i = 0; auto& fence : frameInFlightFences) {
        device.setDebugName(*fence, "frame-in-flight fence (frame #" + std::to_string(i++) + ")");
    }
    for (ui32 i = 0; auto& sem : swapchainImageAcquireSemaphores) {
        device.setDebugName(*sem, "image-acquire semaphore (frame #" + std::to_string(i++) + ")");
    }
    for (ui32 i = 0; auto& sem : renderFinishedSemaphores) {
        device.setDebugName(*sem, "render-finished semaphore (frame #" + std::to_string(i++) + ")");
    }

    // Allocate queues
    drawQueue = tryReserve(device.getQueueManager(), QueueType::graphics);
    device.setDebugName(*drawQueue, "main render graphics queue");
    presentQueue = tryReserve(device.getQueueManager(), QueueType::graphics);
    device.setDebugName(*presentQueue, "main present queue");
}

trc::Renderer::~Renderer() noexcept
{
    waitForAllFrames();
    device.getQueueManager().freeReservedQueue(drawQueue);
    device.getQueueManager().freeReservedQueue(presentQueue);
}

void trc::Renderer::renderFrame(
    u_ptr<Frame> frame,
    vk::Semaphore waitSemaphore,
    vk::Semaphore signalSemaphore)
{
    // Wait for frame
    const auto currentFrameFence = waitForCurrentFrame();

    submitDraw(
        std::move(frame),
        drawQueue,
        waitSemaphore, signalSemaphore,
        currentFrameFence
    );
}

void trc::Renderer::renderFrameAndPresent(
    u_ptr<Frame> frame,
    Swapchain& swapchain)
{
    // Wait for frame
    const auto currentFrameFence = waitForCurrentFrame();

    // Acquire an image from the swapchain
    ui32 image = swapchain.acquireImage(**swapchainImageAcquireSemaphores);

    submitDraw(
        std::move(frame),
        drawQueue,
        **swapchainImageAcquireSemaphores, **renderFinishedSemaphores,
        currentFrameFence
    );

    [[maybe_unused]]
    bool res = swapchain.presentImage(image,
                                      *presentQueue,
                                      { **renderFinishedSemaphores });
}

bool trc::Renderer::waitForAllFrames(ui64 timeoutNs)
{
    std::vector<vk::Fence> fences;
    for (auto& fence : frameInFlightFences) {
        fences.push_back(*fence);
    }
    auto result = device->waitForFences(fences, true, timeoutNs);
    if (result == vk::Result::eTimeout) {
        log::error << "Timeout in Renderer::waitForAllFrames!";
        return false;
    }

    device->waitIdle();  // The fence shit doesn't work, so we do this
    return true;
}

auto trc::Renderer::waitForCurrentFrame() -> vk::Fence
{
    const auto fence = **frameInFlightFences;
    const auto res = device->waitForFences(fence, true, UINT64_MAX);
    if (res == vk::Result::eTimeout) {
        throw std::runtime_error("[In Renderer::drawFrame]: Timeout in waitForFences");
    }
    if (res != vk::Result::eSuccess) {
        log::warn << log::here() << ": Unexpected result from vkWaitForFences: "
                  << vk::to_string(res);
    }

    device->resetFences(fence);

    return fence;
}

void trc::Renderer::submitDraw(
    u_ptr<Frame> frame,
    ExclusiveQueue& queue,
    vk::Semaphore waitSemaphore,
    vk::Semaphore signalSemaphore,
    vk::Fence signalFence)
{
    // Record all draw commands from all scenes to draw
    auto cmdBufs = cmdRecorder.record(*frame);

    // Submit command buffers
    constexpr auto waitStage = vk::PipelineStageFlagBits::eComputeShader
                               | vk::PipelineStageFlagBits::eColorAttachmentOutput;

    std::vector<vk::Semaphore> waitSemaphores;
    std::vector<vk::PipelineStageFlags> waitDstStageMask;
    std::vector<vk::Semaphore> signalSemaphores{
        **renderFinishedHostSignalSemaphores,
    };
    std::vector<ui64> signalSemaphoreValues {
        *renderFinishedHostSignalValue,
    };
    if (waitSemaphore != VK_NULL_HANDLE)
    {
        waitSemaphores.emplace_back(waitSemaphore);
        waitDstStageMask.emplace_back(waitStage);
    }
    if (signalSemaphore != VK_NULL_HANDLE)
    {
        signalSemaphores.emplace_back(signalSemaphore);
        signalSemaphoreValues.emplace_back(0);
    }

    vk::StructureChain submit{
        vk::SubmitInfo(
            waitSemaphores,
            waitDstStageMask,
            cmdBufs,
            signalSemaphores
        ),
        vk::TimelineSemaphoreSubmitInfo({}, signalSemaphoreValues),
    };

    queue.waitSubmit(submit.get(), signalFence);

    // Dispatch asynchronous handler for when the frame has finished rendering
    threadPool.async(RenderFinishedHandler{
        device,
        **renderFinishedHostSignalSemaphores,
        *renderFinishedHostSignalValue,
        std::move(frame)
    });

    ++*renderFinishedHostSignalValue;
}

void trc::Renderer::RenderFinishedHandler::operator()()
{
    auto result = device->waitSemaphores(vk::SemaphoreWaitInfo({}, waitSem, waitVal), UINT64_MAX);
    assert(result == vk::Result::eSuccess);

    frame->signalRenderFinished();
}
