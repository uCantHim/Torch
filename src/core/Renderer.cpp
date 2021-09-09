#include "Renderer.h"

#include <vkb/util/Timer.h>

#include "Window.h"
#include "DrawConfiguration.h"
#include "Scene.h"
#include "TorchResources.h"
#include "util/Util.h"



/////////////////////////
//      Renderer       //
/////////////////////////

trc::Renderer::Renderer(Window& _window)
    :
    instance(_window.getInstance()),
    device(instance.getDevice()),
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
    std::cout << "--- Main presentation family: " << mainPresentQueueFamily << "\n";

    commandCollector = std::make_unique<CommandCollector>(_window, mainRenderQueueFamily);

    swapchainRecreateListener = vkb::on<vkb::PreSwapchainRecreateEvent>([this](auto e) {
        if (e.swapchain != &window->getSwapchain()) return;
        waitForAllFrames();
    }).makeUnique();
}

trc::Renderer::~Renderer()
{
    waitForAllFrames();
}

void trc::Renderer::drawFrame(const DrawConfig& draw)
{
    assert(draw.scene != nullptr);
    assert(draw.camera != nullptr);
    assert(draw.renderConfig != nullptr);

    RenderConfig& renderConfig = *draw.renderConfig;
    RenderGraph& renderGraph = draw.renderConfig->getGraph();

    // Update
    renderConfig.preDraw(draw);

    // Wait for frame
    auto fenceResult = device->waitForFences(**frameInFlightFences, true, 1000000000);
    if (fenceResult == vk::Result::eTimeout) {
        return;
    }
    device->resetFences(**frameInFlightFences);

    // Acquire image
    auto image = window->getSwapchain().acquireImage(**imageAcquireSemaphores);

    // Collect commands from scene
    auto cmdBufs = commandCollector->recordScene(draw, renderGraph);

    // Post-draw cleanup callback
    renderConfig.postDraw(draw);

    // Submit command buffers
    vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eVertexInput;
    mainRenderQueue.submit(
        vk::SubmitInfo(
            **imageAcquireSemaphores,
            waitStage,
            cmdBufs,
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
}
