#pragma once

#include <atomic>
#include <mutex>
#include <vector>

#include "Types.h"
#include "TypesafeId.h"

namespace nc::data
{

class IdPool
{
public:
    auto generate() -> ui64;
    void free(ui64 id);

    void reset();

private:
    std::atomic<ui64> nextId{ 0 };

    std::mutex freeIdsLock;
    std::vector<ui64> freeIds;
};

template<typename Derived>
class HasID
{
public:
    using ID = TypesafeID<Derived, ui64>;
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
    other._id = ~ui64(0);
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
    other._id = ~ui64(0);
    return *this;
}

template<typename Derived>
inline auto HasID<Derived>::getID() const noexcept -> ID
{
    return _id;
}

} // namespace nc::data
