#pragma once

#include <vector>

#include "../Types.h"

namespace trc::util
{
    auto optimizeTriangleOrderingForsyth(const std::vector<ui32>& indices)
        -> std::vector<ui32>;
} // namespace trc::util
