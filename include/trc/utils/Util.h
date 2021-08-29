#pragma once

#include <type_traits>
#include <vector>

#include <vkb/QueueManager.h>

namespace util
{
    template<typename T, typename U>
    inline constexpr T pad(T val, U padding)
    {
        static_assert(std::is_integral_v<T>, "Type must be integral type!");
        static_assert(std::is_integral_v<U>, "Type must be integral type!");

        auto remainder = val % static_cast<T>(padding);
        if (remainder == 0)
            return val;
        else
            return val + (static_cast<T>(padding) - remainder);
    }

    template<typename T>
    inline constexpr T pad_16(T val)
    {
        return pad(val, static_cast<T>(16));
    }

    template<typename T>
    inline constexpr size_t sizeof_pad_16()
    {
        return pad_16(sizeof(T));
    }

    template<size_t Value>
    constexpr auto pad_16_v = pad_16(Value);

    template<typename T>
    constexpr auto sizeof_pad_16_v = sizeof_pad_16<T>();

    template<typename T>
    auto merged(const std::vector<T>& a, const std::vector<T>& b)
        -> std::vector<T>
    {
        if (a.size() > b.size())
        {
            auto result = a;
            result.insert(a.end(), b.begin(), b.end());
            return result;
        }
        else
        {
            auto result = b;
            result.insert(b.end(), a.begin(), a.end());
            return result;
        }
    }

    /**
     * Try to reserve a queue. Order:
     *  1. Reserve primary queue
     *  2. Reserve any queue
     *  3. Don't reserve, just return any queue
     */
    inline auto tryReserve(vkb::QueueManager& qm, vkb::QueueType type)
        -> std::pair<vkb::ExclusiveQueue, vkb::QueueFamilyIndex>
    {
        if (qm.getPrimaryQueueCount(type) > 1)
        {
            return { qm.reservePrimaryQueue(type), qm.getPrimaryQueueFamily(type) };
        }
        else if (qm.getAnyQueueCount(type) > 1)
        {
            auto [queue, family] = qm.getAnyQueue(type);
            return { qm.reserveQueue(queue), family };
        }
        else {
            return qm.getAnyQueue(type);
        }
    };
}
