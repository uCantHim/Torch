#include "data/ObjectId.h"



auto nc::data::IdPool::generate() -> ui64
{
    if (freeIds.empty()) {
        return nextId++;
    }

    std::lock_guard lock(freeIdsLock);
    if (freeIds.empty()) {
        return nextId++;
    }
    else {
        return pop_back(freeIds);
    }
}

void nc::data::IdPool::free(ui64 id)
{
    std::lock_guard lock(freeIdsLock);
    freeIds.push_back(id);
}

void nc::data::IdPool::reset()
{
    std::lock_guard lock(freeIdsLock);
    freeIds.clear();
    nextId = 0;
}
