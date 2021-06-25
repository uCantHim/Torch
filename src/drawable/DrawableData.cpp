#include "drawable/DrawableData.h"



auto trc::DrawableDataStore::create(Node& node) -> ui32
{
    const ui32 id = idPool.generate();
    if (id >= data.size() * ARRAY_SIZE) {
        data.emplace_back(new std::array<DrawableData, ARRAY_SIZE>);
    }

    data[size_t(id / ARRAY_SIZE)]->at(id % ARRAY_SIZE) = { .node=&node };
    return id;
}

void trc::DrawableDataStore::free(ui32 id)
{
    idPool.free(id);
}

auto trc::DrawableDataStore::get(ui32 id) -> DrawableData&
{
    assert(id < data.size() * ARRAY_SIZE);
    return data[size_t(id / ARRAY_SIZE)]->at(id % ARRAY_SIZE);
}
