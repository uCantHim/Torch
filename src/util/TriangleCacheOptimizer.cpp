#include "util/TriangleCacheOptimizer.h"

#include "forsyth.cpp"

#include "trc/Types.h"



namespace trc::util
{

auto optimizeTriangleOrderingForsyth(const std::vector<ui32>& indices) -> std::vector<ui32>
{
    assert(indices.size() % 3 == 0);
    ui32* result = reorderForsyth(indices.data(), indices.size() / 3, indices.size());

    std::vector<ui32> vec(indices.size());
    memcpy(vec.data(), result, sizeof(ui32) * indices.size());
    delete[] result;
    return vec;
}

} // namespace trc::util
