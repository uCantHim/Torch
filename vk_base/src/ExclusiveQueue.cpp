#include "ExclusiveQueue.h"

#include <string>
#include <sstream>



vkb::ExclusiveQueue::ExclusiveQueue(vk::Queue queue)
    :
    queue(queue)
{
}

auto vkb::ExclusiveQueue::operator<<(const vk::SubmitInfo& submit) -> ExclusiveQueue&
{
    this->submit(submit, {});
    return *this;
}

void vkb::ExclusiveQueue::submit(
    const vk::ArrayProxy<const vk::SubmitInfo>& submits,
    vk::Fence fence) const
{
    std::lock_guard lock(sync->submissionLock);
    if (std::this_thread::get_id() != sync->currentThread)
    {
        std::stringstream ss;
        ss << "Tried to submit work in thread " << std::this_thread::get_id()
            << ", but queue is currently owned by thread " << sync->currentThread;
        throw std::runtime_error(ss.str());
    }

    doSubmit(submits, fence);
}

bool vkb::ExclusiveQueue::trySubmit(
    const vk::ArrayProxy<const vk::SubmitInfo>& submits,
    vk::Fence fence) const
{
    std::lock_guard lock(sync->submissionLock);
    if (std::this_thread::get_id() != sync->currentThread) {
        return false;
    }

    doSubmit(submits, fence);
    return true;
}

void vkb::ExclusiveQueue::waitSubmit(
    const vk::ArrayProxy<const vk::SubmitInfo>& submits,
    vk::Fence fence)
{
    std::lock_guard lock(sync->submissionLock);
    transferOwnership(std::this_thread::get_id());
    doSubmit(submits, fence);
}

void vkb::ExclusiveQueue::transferOwnership(std::thread::id newOwner)
{
    if (newOwner != sync->currentThread)
    {
        sync->currentThread = newOwner;
        queue.waitIdle();
    }
}

bool vkb::ExclusiveQueue::hasOwnership(std::thread::id thread) const noexcept
{
    return sync->currentThread == thread;
}

void vkb::ExclusiveQueue::doSubmit(
    const vk::ArrayProxy<const vk::SubmitInfo>& submits,
    vk::Fence fence) const
{
    queue.submit(submits, fence);
}

void vkb::ExclusiveQueue::waitIdle() const
{
    queue.waitIdle();
}
