#pragma once

#include <memory>
#include <thread>
#include <mutex>

#include "trc/VulkanInclude.h"

namespace trc
{
    class ExclusiveQueue
    {
    public:
        /**
         * @brief Create an uninitialized queue
         */
        ExclusiveQueue() = default;

        /**
         * @brief Fully initialize the queue
         */
        explicit ExclusiveQueue(vk::Queue queue);

        ~ExclusiveQueue() = default;

        ExclusiveQueue(const ExclusiveQueue&) = default;
        ExclusiveQueue(ExclusiveQueue&&) noexcept = default;
        auto operator=(const ExclusiveQueue&) -> ExclusiveQueue& = default;
        auto operator=(ExclusiveQueue&&) noexcept -> ExclusiveQueue& = default;

        auto operator<=>(const ExclusiveQueue&) const = default;

        /**
         * @brief Get the underlying vk::Queue
         */
        inline auto operator*() const noexcept -> vk::Queue {
            return queue;
        }

        inline auto operator->() const noexcept -> const vk::Queue* {
            return &queue;
        }

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
        struct SyncState
        {
            mutable std::mutex submissionLock;
            std::thread::id currentThread{ std::this_thread::get_id() };
        };

        void doSubmit(const vk::ArrayProxy<const vk::SubmitInfo>& submits, vk::Fence fence) const;

        vk::Queue queue;
        std::shared_ptr<SyncState> sync{ new SyncState };
    };
} // namespace trc
