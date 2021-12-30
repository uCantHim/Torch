#pragma once

#include <atomic>
#include <mutex>
#include <vector>

#include "TypesafeId.h"

namespace trc::data
{

class IdPool
{
public:
    auto generate() -> uint64_t;
    void free(uint64_t id);

    void reset();

private:
    std::atomic<uint64_t> nextId{ 0 };

    std::mutex freeIdsLock;
    std::vector<uint64_t> freeIds;
};

template<typename Derived>
class HasID
{
public:
    using ID = TypesafeID<Derived, uint64_t>;
    using id_type = ID;

    inline HasID();
    inline HasID(HasID&& other);
    inline ~HasID();

    auto operator=(HasID&& other) -> HasID&;

    HasID(const HasID&) = delete;
    HasID& operator=(const HasID&) = delete;

    inline auto getID() const noexcept -> ID;

    static inline void resetIdPool() {
        nextId.reset();
    }

private:
    static inline IdPool nextId;

    ID _id;
};



// -------------------------------------- //
//      Template member definitions       //
// -------------------------------------- //

template<typename Derived>
inline HasID<Derived>::HasID()
    : _id(nextId.generate())
{
}

template<typename Derived>
inline HasID<Derived>::HasID(HasID&& other)
    : _id(other._id)
{
    other._id = ~uint64_t(0);
}

template<typename Derived>
inline HasID<Derived>::~HasID()
{
    nextId.free(_id);
}

template<typename Derived>
inline auto HasID<Derived>::operator=(HasID&& other) -> HasID&
{
    _id = other._id;
    other._id = ~uint64_t(0);
    return *this;
}

template<typename Derived>
inline auto HasID<Derived>::getID() const noexcept -> ID
{
    return _id;
}

} // namespace trc::data
