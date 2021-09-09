#include "util/data/ObjectId.h"



auto trc::data::IdPool::generate() -> ui64
{
    if (freeIds.empty()) {
        return nextId++;
    }

    std::lock_guard lock(freeIdsLock);
    if (freeIds.empty()) {
        return nextId++;
    }
    else {
        auto result = freeIds.back();
        freeIds.pop_back();
        return result;
    }
}

void trc::data::IdPool::free(ui64 id)
{
    std::lock_guard lock(freeIdsLock);
    freeIds.push_back(id);
}

void trc::data::IdPool::reset()
{
    std::lock_guard lock(freeIdsLock);
    freeIds.clear();
    nextId = 0;
}
