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
    IdPool(const IdPool&) = delete;
    IdPool& operator=(const IdPool&) = delete;

    IdPool() = default;
    ~IdPool() noexcept = default;

    IdPool(IdPool&& other) noexcept
        : nextId(T{other.nextId}), freeIds(std::move(other.freeIds))
    {
        other.nextId = T{0};
    }

    IdPool& operator=(IdPool&& rhs) noexcept
    {
        if (this != &rhs)
        {
            nextId = T{rhs.nextId};
            rhs.nextId = 0;
            freeIds = std::move(rhs.freeIds);
        }
        return *this;
    }

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
