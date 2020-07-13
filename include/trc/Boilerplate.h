#pragma once

#include <vulkan/vulkan.hpp>

#define GLM_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
#include <glm/glm.hpp>
using namespace glm;

namespace trc
{
    using int8 = int8_t;
    using int16 = int16_t;
    using int32 = int32_t;
    using int64 = int64_t;

    using i8 = int8;
    using i16 = int16;
    using i32 = int32;
    using i64 = int64;

    using uint8 = uint8_t;
    using uint16 = uint16_t;
    using uint32 = uint32_t;
    using uint64 = uint64_t;

    using ui8 = uint8;
    using ui16 = uint16;
    using ui32 = uint32;
    using ui64 = uint64;
} // namespace trc
