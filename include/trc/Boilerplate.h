#pragma once

#include <vulkan/vulkan.hpp>

#define GLM_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#define TRC_FLIP_Y_PROJECTION

namespace trc::basic_types
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

    using bool32 = ui32;
} // namespace trc::basic_types

namespace trc
{
    using namespace glm;
    using namespace basic_types;
}
