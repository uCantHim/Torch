#include "trc/core/FrameRenderState.h"



void trc::FrameRenderState::onRenderFinished(std::function<void()> func)
{
    std::scoped_lock lock(mutex);
    renderFinishedCallbacks.emplace_back(std::move(func));
}

auto trc::FrameRenderState::makeTransientBuffer(
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

void trc::FrameRenderState::signalRenderFinished()
{
    std::scoped_lock lock(mutex);
    for (auto& func : renderFinishedCallbacks) {
        func();
    }
    transientBuffers.clear();
}
