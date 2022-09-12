#pragma once

#include <trc/Types.h>
using namespace trc::basic_types;

enum class Axis : ui32
{
    eNone = 0,
    eX = 1 << 0,
    eY = 1 << 1,
    eZ = 1 << 2,
};

namespace vk
{
    template <>
    struct FlagTraits<::Axis>
    {
        enum : VkFlags
        {
            allFlags = VkFlags(::Axis::eX) | VkFlags(::Axis::eY) | VkFlags(::Axis::eZ)
        };
    };
} // namespace vk

using AxisFlags = vk::Flags<Axis>;

inline auto toVector(AxisFlags flags) -> vec3
{
    return { !(flags & Axis::eX), !(flags & Axis::eY), !(flags & Axis::eZ) };
}

VULKAN_HPP_INLINE VULKAN_HPP_CONSTEXPR
    AxisFlags operator|( Axis bit0,
                         Axis bit1 ) VULKAN_HPP_NOEXCEPT
{
    return AxisFlags( bit0 ) | bit1;
}

VULKAN_HPP_INLINE VULKAN_HPP_CONSTEXPR
    AxisFlags operator&( Axis bit0,
                         Axis bit1) VULKAN_HPP_NOEXCEPT
{
    return AxisFlags( bit0 ) & bit1;
}

VULKAN_HPP_INLINE VULKAN_HPP_CONSTEXPR
    AxisFlags operator^( Axis bit0,
                         Axis bit1 ) VULKAN_HPP_NOEXCEPT
{
    return AxisFlags( bit0 ) ^ bit1;
}

VULKAN_HPP_INLINE VULKAN_HPP_CONSTEXPR
    AxisFlags operator~( Axis bits ) VULKAN_HPP_NOEXCEPT
{
    return ~( AxisFlags( bits ) );
}
