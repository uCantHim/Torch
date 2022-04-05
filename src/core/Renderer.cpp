#include "Renderer.h"

#include <trc_util/Util.h>
#include <trc_util/Timer.h>
#include <trc_util/algorithm/VectorTransform.h>

#include "Window.h"
#include "DrawConfiguration.h"
#include "RenderConfiguration.h"
#include "TorchResources.h"
#include "FrameRenderState.h"

#include "Scene.h"  // TODO: Remove reference to specialized type from core



/**
 * Try to reserve a queue. Order:
 *  1. Reserve primary queue
 *  2. Reserve any queue
 *  3. Don't reserve, just return any queue
 */
inline auto tryReserve(vkb::QueueManager& qm, vkb::QueueType type)
    -> std::pair<vkb::ExclusiveQueue, vkb::QueueFamilyIndex>
{
    if (qm.getPrimaryQueueCount(type) > 1)
    {
        return { qm.reservePrimaryQueue(type), qm.getPrimaryQueueFamily(type) };
    }
    else if (qm.getAnyQueueCount(type) > 1)
    {
        auto [queue, family] = qm.getAnyQueue(type);
        return { qm.reserveQueue(queue), family };
    }
    else {
        return qm.getAnyQueue(type);
    }
};



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
    hostSemSignalValue(_window, [](ui32){ return 1; })
{
    createSemaphores();

    vkb::QueueManager& qm = _window.getDevice().getQueueManager();
    std::tie(mainRenderQueue, mainRenderQueueFamily) = tryReserve(qm, vkb::QueueType::graphics);
    std::tie(mainPresentQueue, mainPresentQueueFamily) = tryReserve(qm, vkb::QueueType::presentation);

    if constexpr (vkb::enableVerboseLogging)
    {
        std::cout << "--- Main render family for renderer: " << mainRenderQueueFamily << "\n";
        std::cout << "--- Main presentation family for renderer: " << mainPresentQueueFamily << "\n";
    }

    swapchainRecreateListener = vkb::on<vkb::PreSwapchainRecreateEvent>([this](auto e) {
        if (e.swapchain == window) {
            waitForAllFrames();
        }
    }).makeUnique();
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
    auto fenceResult = device->waitForFences(**frameInFlightFences, true, UINT64_MAX);
    if (fenceResult == vk::Result::eTimeout) {
        throw std::runtime_error("[In Renderer::drawFrame]: Timeout in waitForFences");
    }
    device->resetFences(**frameInFlightFences);

    // Acquire image
    auto image = window->acquireImage(**imageAcquireSemaphores);

    // Record commands
    auto frameState = std::make_shared<FrameRenderState>();

    std::vector<vk::CommandBuffer> commandBuffers;
    for (const auto& draw : draws)
    {
        assert(draw.scene != nullptr);
        assert(draw.camera != nullptr);
        assert(draw.renderConfig != nullptr);

        RenderConfig& renderConfig = *draw.renderConfig;

        // Update
        renderConfig.preDraw(draw);

        // Collect commands from scene
        auto cmdBufs = renderConfig.getLayout().record(
            *draw.renderConfig,
            *draw.scene,
            *frameState
        );
        util::merge(commandBuffers, cmdBufs);

        // Post-draw cleanup callback
        renderConfig.postDraw(draw);
    }

    // Submit command buffers
    vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eComputeShader
                                       | vk::PipelineStageFlagBits::eColorAttachmentOutput;

    std::vector<vk::Semaphore> signalSemaphores{
        **renderFinishedSemaphores,
        **renderFinishedHostSignalSemaphores,
    };
    const ui64 signalValues[]{ 0, *hostSemSignalValue };
    vk::StructureChain chain{
        vk::SubmitInfo(
            **imageAcquireSemaphores,
            waitStage,
            commandBuffers,
            signalSemaphores
        ),
        vk::TimelineSemaphoreSubmitInfo(0, nullptr, 2, signalValues)
    };
    mainRenderQueue.submit(chain.get(), **frameInFlightFences);

    // Dispatch asynchronous handler for when the frame has finished rendering
    threadPool.async(
        [
            this,
            sem=**renderFinishedHostSignalSemaphores,
            val=*hostSemSignalValue,
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

    ++*hostSemSignalValue;

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

    threadPool.trim();
}
