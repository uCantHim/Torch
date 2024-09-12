#include "trc/core/Frame.h"

#include <trc_util/Assert.h>

#include "trc/base/Buffer.h"



namespace trc
{

void FrameRenderState::onRenderFinished(std::function<void()> func)
{
    std::scoped_lock lock(mutex);
    renderFinishedCallbacks.emplace_back(std::move(func));
}

auto FrameRenderState::makeTransientBuffer(
    const Device& device,
    size_t size,
    vk::BufferUsageFlags usageFlags,
    vk::MemoryPropertyFlags memoryFlags,
    const DeviceMemoryAllocator& alloc) -> Buffer&
{
    std::scoped_lock lock(mutex);
    return *transientBuffers.emplace_back(
        std::make_unique<Buffer>(device, size, usageFlags, memoryFlags, alloc)
    );
}

void FrameRenderState::signalRenderFinished()
{
    std::scoped_lock lock(mutex);
    for (auto& func : renderFinishedCallbacks) {
        func();
    }
    transientBuffers.clear();
}



Frame::Frame(
    const Device& device,
    RenderGraphLayout renderGraph,
    s_ptr<ResourceStorage> resources)
    :
    device(device),
    renderGraph(std::move(renderGraph)),
    resources(std::move(resources))
{
    assert_arg(this->resources != nullptr);
}

auto Frame::getDevice() const -> const Device&
{
    return device;
}

auto Frame::getRenderGraph() const -> const RenderGraphLayout&
{
    return renderGraph;
}

auto Frame::getResources() -> ResourceStorage&
{
    return *resources;
}

auto Frame::getTaskQueue() -> DeviceTaskQueue&
{
    return taskQueue;
}

auto Frame::makeTaskExecutionContext(s_ptr<DependencyRegion> depRegion) &
    -> DeviceExecutionContext
{
    return DeviceExecutionContext{
        *this,
        std::move(depRegion),
        resources
    };
}

void Frame::spawnTask(RenderStage::ID stage, u_ptr<DeviceTask> task)
{
    taskQueue.spawnTask(stage, std::move(task));
}

} // namespace trc
