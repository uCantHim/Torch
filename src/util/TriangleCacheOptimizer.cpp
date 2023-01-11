#include "trc/util/TriangleCacheOptimizer.h"

#include "forsyth.cpp"
#include "trc/Types.h"
#include "trc/base/Logging.h"



namespace trc::util
{

auto optimizeTriangleOrderingForsyth(const std::vector<ui32>& indices) -> std::vector<ui32>
{
    if (indices.size() % 3 != 0)
    {
        log::warn << "Unable to optimize triangle ordering: number of indices ("
                  << indices.size() << ") is not a multiple of three. This can occur when the"
                  << " mesh is not properly triangulated.";
        return indices;
    }

    ui32* result = reorderForsyth(indices.data(), indices.size() / 3, indices.size());
    if (result == nullptr)
    {
        throw std::invalid_argument("[In optimizeTriangleOrderingForsyth]: Unsupported mesh. At"
                                    " least one vertex is shared by more than "
                                    + std::to_string(MAX_ADJACENCY) + " triangles.");
    }

    std::vector<ui32> vec(indices.size());
    memcpy(vec.data(), result, sizeof(ui32) * indices.size());
    delete[] result;
    return vec;
}

} // namespace trc::util
