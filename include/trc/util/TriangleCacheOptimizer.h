#pragma once

#include <vector>

#include "trc/Types.h"

namespace trc::util
{
    auto optimizeTriangleOrderingForsyth(const std::vector<ui32>& indices)
        -> std::vector<ui32>;
} // namespace trc::util
