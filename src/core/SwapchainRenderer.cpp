#include "trc/core/SwapchainRenderer.h"

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

SwapchainRenderer::SwapchainRenderer(Device& _device, Swapchain& _swapchain)
    :
    device(&_device),
    swapchain(&_swapchain),
    renderer(_device, *swapchain),
    imageAcquireSemaphores(*swapchain),
    renderFinishedSemaphores(*swapchain)
{
    imageAcquireSemaphores = {
        *swapchain,
        [&](ui32) { return device->get().createSemaphoreUnique({}); }
    };
    renderFinishedSemaphores = {
        *swapchain,
        [&](ui32) { return device->get().createSemaphoreUnique({}); }
    };

    for (ui32 i = 0; auto& sem : imageAcquireSemaphores) {
        device->setDebugName(*sem, "image-acquire semaphore (frame #" + std::to_string(i++) + ")");
    }
    for (ui32 i = 0; auto& sem : renderFinishedSemaphores) {
        device->setDebugName(*sem, "render-finished semaphore (frame #" + std::to_string(i++) + ")");
    }

    mainRenderQueue = tryReserve(device->getQueueManager(), QueueType::graphics);
    device->setDebugName(*mainRenderQueue, "main render graphics queue");
    mainPresentQueue = tryReserve(device->getQueueManager(), QueueType::graphics);
    device->setDebugName(*mainPresentQueue, "main present queue");
}

SwapchainRenderer::~SwapchainRenderer() noexcept
{
    device->getQueueManager().freeReservedQueue(mainRenderQueue);
    device->getQueueManager().freeReservedQueue(mainPresentQueue);
}

void SwapchainRenderer::renderFrame(u_ptr<Frame> _frame)
{
    s_ptr<Frame> frame{ std::move(_frame) };

    // Acquire an image from the swapchain
    // TODO: Change current image of FrameClock in acquire, not in present.
    ui32 image = swapchain->acquireImage(**imageAcquireSemaphores);

    renderer.renderFrame(frame,
                       mainRenderQueue,
                       **imageAcquireSemaphores,
                       **renderFinishedSemaphores);

    [[maybe_unused]]
    bool res = swapchain->presentImage(image,
                                       *mainPresentQueue,
                                       { **renderFinishedSemaphores });
}

void SwapchainRenderer::waitForAllFrames(ui64 timeoutNanoseconds)
{
    renderer.waitForAllFrames(timeoutNanoseconds);
}

} // namespace trc
