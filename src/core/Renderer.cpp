#include "Renderer.h"

#include <trc_util/Util.h>
#include <trc_util/algorithm/VectorTransform.h>

#include "Window.h"
#include "DrawConfiguration.h"
#include "RenderConfiguration.h"
#include "TorchResources.h"



/////////////////////////
//      Renderer       //
/////////////////////////

trc::Renderer::Renderer(Window& _window)
    :
    instance(_window.getInstance()),
    device(_window.getDevice()),
    window(&_window),
    imageAcquireSemaphores(_window.getSwapchain()),
    renderFinishedSemaphores(_window.getSwapchain()),
    frameInFlightFences(_window.getSwapchain())
{
    createSemaphores();

    vkb::QueueManager& qm = _window.getDevice().getQueueManager();
    std::tie(mainRenderQueue, mainRenderQueueFamily)
        = util::tryReserve(qm, vkb::QueueType::graphics);
    std::tie(mainPresentQueue, mainPresentQueueFamily)
        = util::tryReserve(qm, vkb::QueueType::presentation);

    if constexpr (vkb::enableVerboseLogging)
    {
        std::cout << "--- Main render family for renderer: " << mainRenderQueueFamily << "\n";
        std::cout << "--- Main presentation family for renderer: " << mainPresentQueueFamily << "\n";
    }

    swapchainRecreateListener = vkb::on<vkb::PreSwapchainRecreateEvent>([this](auto e) {
        if (e.swapchain != &window->getSwapchain()) return;
        waitForAllFrames();
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
        return;
    }
    device->resetFences(**frameInFlightFences);

    // Acquire image
    auto image = window->getSwapchain().acquireImage(**imageAcquireSemaphores);

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
        auto cmdBufs = draw.renderConfig->getLayout().record(draw);
        util::merge(commandBuffers, cmdBufs);

        // Post-draw cleanup callback
        renderConfig.postDraw(draw);
    }

    // Submit command buffers
    vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eComputeShader;
    mainRenderQueue.submit(
        vk::SubmitInfo(
            **imageAcquireSemaphores,
            waitStage,
            commandBuffers,
            **renderFinishedSemaphores
        ),
        **frameInFlightFences
    );

    // Present frame
    window->getSwapchain().presentImage(image, *mainPresentQueue, { **renderFinishedSemaphores });
}

void trc::Renderer::createSemaphores()
{
    imageAcquireSemaphores = {
        window->getSwapchain(),
        [this](ui32) { return device->createSemaphoreUnique({}); }
    };
    renderFinishedSemaphores = {
        window->getSwapchain(),
        [this](ui32) { return device->createSemaphoreUnique({}); }
    };
    frameInFlightFences = {
        window->getSwapchain(),
        [this](ui32) {
            return device->createFenceUnique(
                { vk::FenceCreateFlagBits::eSignaled }
            );
        }
    };
}

void trc::Renderer::waitForAllFrames(std::chrono::nanoseconds timeout)
{
    std::vector<vk::Fence> fences;
    for (auto& fence : frameInFlightFences) {
        fences.push_back(*fence);
    }
    auto result = device->waitForFences(fences, true, timeout.count());
    if (result != vk::Result::eSuccess)
    {
        throw Exception("[In Renderer::waitForAllFrames]: Result from vkWaitForFences was"
                        " expected to be " + vk::to_string(vk::Result::eSuccess)
                        + ", but was " + vk::to_string(result) + "!");
    }
}
