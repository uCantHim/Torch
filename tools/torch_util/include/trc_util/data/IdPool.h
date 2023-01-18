#pragma once

#include <atomic>

#include "ThreadsafeQueue.h"

namespace trc::data
{

/**
 * @brief Thread-safe generator for reusable IDs
 */
template<typename T = uint64_t>
class IdPool
{
public:
    auto generate() -> T
    {
        if (auto id = freeIds.try_pop()) {
            return *id;
        }
        return nextId++;
    }

    void free(T id)
    {
        freeIds.push(id);
    }

    void reset()
    {
        freeIds = {};
        nextId = 0;
    }

private:
    std::atomic<T> nextId{ T(0) };
    ThreadsafeQueue<T> freeIds;
};

} // namespace trc::data
