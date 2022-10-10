#pragma once

#include <vector>

#include "trc/Types.h"

namespace trc::util
{
    /**
     * @throw std::invalid_argument if the mesh type is not supported
     */
    auto optimizeTriangleOrderingForsyth(const std::vector<ui32>& indices)
        -> std::vector<ui32>;
} // namespace trc::util
