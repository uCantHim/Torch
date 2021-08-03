#include "Renderer.h"

#include <vkb/util/Timer.h>

#include "Window.h"
#include "DrawConfiguration.h"
#include "Scene.h"
#include "TorchResources.h"



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

    Scene& scene = *draw.scene;
    RenderConfig& renderConfig = *draw.renderConfig;
    RenderGraph& renderGraph = draw.renderConfig->getGraph();

    if (draw.renderAreas.empty()) return;

    // Ensure that enough command collectors are available
    while (renderGraph.size() > commandCollectors.size()) {
        commandCollectors.emplace_back(instance, *window);
    }

    // Update
    scene.update();
    renderConfig.preDraw(draw);

    // Acquire image
    auto fenceResult = device->waitForFences(**frameInFlightFences, true, UINT64_MAX);
    assert(fenceResult == vk::Result::eSuccess);
    device->resetFences(**frameInFlightFences);
    auto image = window->getSwapchain().acquireImage(**imageAcquireSemaphores);

    // Collect commands from scene
    int collectorIndex{ 0 };
    std::vector<vk::CommandBuffer> cmdBufs;
    renderGraph.foreachStage(
        [&, this](RenderStageType::ID stage, const std::vector<RenderPass*>& passes)
        {
            auto& collector = commandCollectors.at(collectorIndex);
            cmdBufs.push_back(collector.recordScene(draw, stage, passes));

            collectorIndex++;
        }
    );

    // Post-draw cleanup callback
    renderConfig.postDraw(draw);

    // Submit command buffers
    auto& graphicsQueue = Queues::getMainRender();
    vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eVertexInput;
    graphicsQueue.submit(
        vk::SubmitInfo(
            **imageAcquireSemaphores,
            waitStage,
            cmdBufs,
            **renderFinishedSemaphores
        ),
        **frameInFlightFences
    );

    // Present frame
    auto& presentQueue = Queues::getMainPresent();
    window->getSwapchain().presentImage(image, *presentQueue, { **renderFinishedSemaphores });
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
