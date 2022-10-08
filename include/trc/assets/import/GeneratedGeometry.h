#pragma once

#include "trc/assets/Geometry.h"

namespace trc
{
    auto makePlaneGeo(
        float width = 1.0f,
        float height = 1.0f,
        ui32 segmentsX = 1,
        ui32 segmentsZ = 1,
        std::function<float(float, float)> heightFunc = [](...) { return 0.0f; }
    ) -> GeometryData;

    auto makeCubeGeo() -> GeometryData;

    /**
     * @brief Generate a sphere geometry
     *
     * @param size_t columns Number of vertices on any given horizontal
     *                       `2*PI`-long ring.
     * @param size_t rows    Number of vertices on any given vertical
     *                       `PI`-long half-ring. Is usually half as much
     *                       as `columns`.
     */
    auto makeSphereGeo(size_t columns = 32, size_t rows = 16) -> GeometryData;
} // namespace trc
