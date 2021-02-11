#pragma once

#include <queue>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>

#include <vulkan/vulkan.hpp>

namespace vkb
{
    class ExclusiveQueue
    {
    public:
        explicit ExclusiveQueue(vk::Queue queue);
        ~ExclusiveQueue() = default;

        ExclusiveQueue(const ExclusiveQueue&) = delete;
        ExclusiveQueue(ExclusiveQueue&&) noexcept = delete;
        auto operator=(const ExclusiveQueue&) -> ExclusiveQueue& = delete;
        auto operator=(ExclusiveQueue&&) noexcept -> ExclusiveQueue& = delete;

        inline auto operator*() const noexcept -> vk::Queue {
            return queue;
        }

        /**
         * @brief Calls ExclusiveQueue::submit
         *
         * @throw std::runtime_error if current thread doesn't own queue
         */
        auto operator<<(const vk::SubmitInfo& submit) -> ExclusiveQueue&;

        /**
         * Essentially the same behaviour as vkQueueSubmit
         *
         * @throw std::runtime_error if current thread doesn't own queue
         */
        void submit(const vk::ArrayProxy<const vk::SubmitInfo>& submits, vk::Fence fence) const;

        /**
         * Returns false and does nothing if queue is being owned by another
         * thread. Submits the work and return true otherwise.
         */
        bool trySubmit(const vk::ArrayProxy<const vk::SubmitInfo>& submits, vk::Fence fence) const;

        /**
         * Waits until queue is free, then transfers ownership to current
         * thread, then submits work. Blocks submission of new work while
         * waiting.
         *
         * Has the same overhead as `submit` or `trySubmit` when the calling
         * thread already owns the queue.
         *
         * This can never fail but it may wait a while until the queue is
         * free.
         */
        void waitSubmit(const vk::ArrayProxy<const vk::SubmitInfo>& submits, vk::Fence fence);

        /**
         * @brief Transfers queue ownership to current thread
         *
         * Waits until the queue is idle, then changes ownership
         */
        void transferOwnership(std::thread::id newOwner);

        /**
         * @param std::thread::id thread
         *
         * @return bool True if the specified thread currently owns the
         *              queue
         */
        bool hasOwnership(std::thread::id thread) const noexcept;

        void waitIdle() const;

    private:
        void doSubmit(const vk::ArrayProxy<const vk::SubmitInfo>& submits, vk::Fence fence) const;

        vk::Queue queue;

        mutable std::mutex submissionLock;
        std::thread::id currentThread{ std::this_thread::get_id() };
    };
} // namespace vkb
