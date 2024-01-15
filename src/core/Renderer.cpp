#include "trc/core/Renderer.h"

#include <cmath>
#include <vector>

#include "trc/base/Logging.h"
#include "trc/core/Frame.h"



/////////////////////////
//      Renderer       //
/////////////////////////

trc::Renderer::Renderer(const Device& _device, const FrameClock& clock)
    :
    device(_device),
    threadPool(std::max(1u, std::thread::hardware_concurrency() - 1u)),
    cmdRecorder(_device, clock, &threadPool),
    frameInFlightFences(clock),
    renderFinishedHostSignalSemaphores(clock),
    renderFinishedHostSignalValue(clock, [](ui32){ return 1; })
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

    for (ui32 i = 0; auto& sem : renderFinishedHostSignalSemaphores) {
        device.setDebugName(*sem, "host-signal-on-render-finished semaphore (frame #" + std::to_string(i++) + ")");
    }
    for (ui32 i = 0; auto& fence : frameInFlightFences) {
        device.setDebugName(*fence, "frame-in-flight fence (frame #" + std::to_string(i++) + ")");
    }

}

trc::Renderer::~Renderer() noexcept
{
    waitForAllFrames();
}

void trc::Renderer::renderFrame(
    s_ptr<Frame> frame,
    ExclusiveQueue& queue,
    vk::Semaphore waitSemaphore,
    vk::Semaphore signalSemaphore)
{
    // Wait for frame
    const auto currentFrameFence = **frameInFlightFences;
    const auto fenceResult = device->waitForFences(currentFrameFence, true, UINT64_MAX);
    if (fenceResult == vk::Result::eTimeout) {
        throw std::runtime_error("[In Renderer::drawFrame]: Timeout in waitForFences");
    }

    // Record all draw commands from all scenes to draw
    auto cmdBufs = cmdRecorder.record(*frame);

    // Submit command buffers
    vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eComputeShader
                                       | vk::PipelineStageFlagBits::eColorAttachmentOutput;

    std::vector<vk::Semaphore> signalSemaphores{
        signalSemaphore,
        **renderFinishedHostSignalSemaphores,
    };
    const ui64 signalValues[]{ 0, *renderFinishedHostSignalValue };
    vk::StructureChain submit{
        vk::SubmitInfo(
            waitSemaphore,
            waitStage,
            cmdBufs,
            signalSemaphores
        ),
        vk::TimelineSemaphoreSubmitInfo(0, nullptr, 2, signalValues)
    };

    device->resetFences(currentFrameFence);
    queue.waitSubmit(submit.get(), currentFrameFence);

    // Dispatch asynchronous handler for when the frame has finished rendering
    threadPool.async(RenderFinishedHandler{
        device,
        **renderFinishedHostSignalSemaphores,
        *renderFinishedHostSignalValue,
        frame
    });

    ++*renderFinishedHostSignalValue;
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

void trc::Renderer::RenderFinishedHandler::operator()()
{
    auto result = device->waitSemaphores(vk::SemaphoreWaitInfo({}, waitSem, waitVal), UINT64_MAX);
    assert(result == vk::Result::eSuccess);

    frame->signalRenderFinished();
}
