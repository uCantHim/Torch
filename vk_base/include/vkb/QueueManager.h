#pragma once

#include <cstdint>
#include <vector>
#include <array>
#include <unordered_set>

#include "basics/PhysicalDevice.h"

namespace vkb
{
    class Device;

    class QueueReservedError : public std::exception
    {
    public:
        explicit QueueReservedError(std::string errorMsg)
            : error(std::move(errorMsg))
        {}

        auto what() const noexcept -> const char* override {
            return error.c_str();
        }

    private:
        std::string error;
    };

    class QueueManager
    {
    public:
        explicit QueueManager(const PhysicalDevice& physDevice, const Device& device);

        /**
         * @brief Query all unreserved queues of a specific queue family
         *
         * @return std::vector<vk::Queue> All unreserved queues of a
         *                                specific queue family.
         */
        auto getFamilyQueues(QueueFamilyIndex family) const -> std::vector<vk::Queue>;

        /**
         * @return QueueFamilyIndex Index of the primary queue family for
         *         the specified capability
         *
         * @throw std::out_of_range if no family supports the requested
         *                          capability.
         */
        auto getPrimaryQueueFamily(QueueType type) const -> QueueFamilyIndex;

        /**
         * Rotates through all primary queues for the requested capability
         *
         * @throw std::out_of_range if no family supports the requested
         *                          capability.
         * @throw QueueReservedError if all primary queues of the requested
         *        capability are reserved.
         */
        auto getPrimaryQueue(QueueType type) const -> vk::Queue;

        /**
         * Query specific queue of primary queue family for a specific
         * capability.
         *
         * @param QueueType type
         * @param uint32_t queueIndex May be in the range
         *                            [0, getPrimaryQueueCount(type) - 1].
         *
         * @throw std::out_of_range if no family supports the requested
         *                          capability or if no queue exists at the
         *                          specified index.
         * @throw QueueReservedError if the primary queue at the requested
         *        index is reserved.
         */
        auto getPrimaryQueue(QueueType type, uint32_t queueIndex) const -> vk::Queue;

        /**
         * @param QueueType type
         *
         * @return uint32_t Number of queues in primary queue family of
         *                  a specific capability. 0 if no family supports
         *                  the requested capability.
         */
        auto getPrimaryQueueCount(QueueType type) const noexcept -> uint32_t;

        /**
         * Rotates through all queues for the requested capability. This
         * means that the first call returns getAnyQueue(type, 0), the call
         * after that returns getAnyQueue(type, 1), etc.
         *
         * @throw std::out_of_range if no family supports the requested
         *                          capability.
         * @throw QueueReservedError if all queues with the requested
         *        capability are reserved.
         */
        auto getAnyQueue(QueueType type) const
            -> std::pair<vk::Queue, QueueFamilyIndex>;

        /**
         * Query specific queue with the requested capability.
         *
         * @param QueueType type
         * @param uint32_t queueIndex May be in the range
         *                            [0, getAnyQueueCount(type) - 1].
         *
         * @throw std::out_of_range if no queue with the specified index
         *                          exists.
         */
        auto getAnyQueue(QueueType type, uint32_t queueIndex) const
            -> std::pair<vk::Queue, QueueFamilyIndex>;

        /**
         * @param QueueType type
         *
         * @return uint32_t Number of queues of any family with a specific
         *         capability
         */
        auto getAnyQueueCount(QueueType type) const noexcept -> uint32_t;

        /**
         * @brief Reserve a queue
         *
         * Reserving a queue locks it and makes it impossible to retrieve
         * the queue from the QueueManager with subsequent query calls.
         *
         * @param vk::Queue queue The queue to reserve.
         *
         * @return vk::Queue The queue that was specified as argument
         *         (purely for convenience)
         *
         * @throw QueueReservedError if the specified queue is already
         *                           reserved.
         * @throw std::out_of_range if the specified queue handle doesn't
         *                          exist.
         */
        auto reserveQueue(vk::Queue queue) -> vk::Queue;

        /**
         * @brief Reserve the next primary queue in the rotation
         *
         * Reserving a queue locks it and makes it impossible to retrieve
         * the queue from the QueueManager with subsequent query calls.
         *
         * Uses the same rotation as getPrimaryQueue does.
         *
         * @throw QueueReservedError if the specified queue is already
         *                           reserved.
         * @throw std::out_of_range if no family supports the requested
         *                          capability.
         */
        auto reservePrimaryQueue(QueueType type) -> vk::Queue;

        /**
         * @brief Reserve a specific primary queue
         *
         * Reserving a queue locks it and makes it impossible to retrieve
         * the queue from the QueueManager with subsequent query calls.
         *
         * @throw QueueReservedError if the specified queue is already
         *                           reserved.
         * @throw std::out_of_range if no family supports the requested
         *                          capability or if no queue exists at the
         *                          specified index.
         */
        auto reservePrimaryQueue(QueueType type, uint32_t queueIndex) -> vk::Queue;

    private:
        static constexpr size_t queueTypeCount = static_cast<size_t>(QueueType::numQueueTypes);

        // All queues. Other data structures index this array.
        std::vector<vk::Queue> queueStorage;
        std::unordered_set<uint32_t> reservedQueueIndices;

        /**
         * @brief Get a queue from storage
         *
         * @return null if the queue has been reserved
         *
         * @throw std::out_of_range if index is out of range
         */
        auto getQueue(uint32_t index) const -> std::optional<vk::Queue>;

        /**
         * Stores a list of queues at queue family indices.
         *
         * `queuesPerFamily[familyIndex]` is the list of the queue family
         * with index `familyIndex`.
         */
        std::vector<std::vector<uint32_t>> queuesPerFamily;

        // Stores one primary queue family for each queue capability
        std::array<QueueFamilyIndex, queueTypeCount> primaryQueueFamilies;

        // Stores any capable queue and its family for each queue capability
        std::array<
            std::vector<
                std::pair<uint32_t, QueueFamilyIndex>
            >,
            queueTypeCount
        > queuesPerCapability;

        /**
         * Stores an index per queue type that indicates the next queue
         * in the rotation of the getPrimaryQueue(QueueType) function.
         */
        mutable std::array<uint32_t, queueTypeCount> nextPrimaryQueueRotation{ 0 };

        /**
         * Like nextPrimaryQueueRotation, but for any queue family.
         */
        mutable std::array<uint32_t, queueTypeCount> nextAnyQueueRotation{ 0 };
    };
} // namespace vkb
