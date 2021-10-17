#include "drawable/DrawableData.h"



namespace trc::legacy
{

auto DrawableDataStore::create(Node& node, AnimationEngine& animEngine) -> ui32
{
    const ui32 id = idPool.generate();
    if (id >= data.size() * ARRAY_SIZE) {
        data.emplace_back(new std::array<DrawableData, ARRAY_SIZE>);
    }

    data[size_t(id / ARRAY_SIZE)]->at(id % ARRAY_SIZE) = {
        .modelMatrixId=node.getGlobalTransformID(),
        .anim=animEngine.getState(),
    };

    return id;
}

void DrawableDataStore::free(ui32 id)
{
    idPool.free(id);
}

auto DrawableDataStore::get(ui32 id) -> DrawableData&
{
    assert(id < data.size() * ARRAY_SIZE);
    return data[size_t(id / ARRAY_SIZE)]->at(id % ARRAY_SIZE);
}

} // namespace trc::legacy
