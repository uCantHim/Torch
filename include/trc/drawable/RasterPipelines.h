#pragma once

#include "Types.h"

#include "core/Pipeline.h"

namespace trc
{
    enum class PipelineFeatureFlagBits : ui64
    {
        eTransparent = 1 << 0,
        eAnimated    = 1 << 1,

        eShadow      = 1 << 3,
    };

    using PipelineFeatureFlags = vk::Flags<PipelineFeatureFlagBits>;

    auto getPipeline(PipelineFeatureFlags featureFlags) -> Pipeline::ID;
    auto getPoolInstancePipeline(PipelineFeatureFlags flags) -> Pipeline::ID;
} // namespace trc

namespace vk
{
    template <>
    struct FlagTraits<trc::PipelineFeatureFlagBits>
    {
      enum : VkFlags
      {
        allFlags = VkFlags(trc::PipelineFeatureFlagBits::eTransparent)
                   | VkFlags(trc::PipelineFeatureFlagBits::eAnimated)
                   | VkFlags(trc::PipelineFeatureFlagBits::eShadow)
      };
    };
} // namespace vk

namespace trc
{
    VULKAN_HPP_INLINE VULKAN_HPP_CONSTEXPR
        PipelineFeatureFlags operator|( PipelineFeatureFlagBits bit0,
                                        PipelineFeatureFlagBits bit1 ) VULKAN_HPP_NOEXCEPT
    {
        return PipelineFeatureFlags( bit0 ) | bit1;
    }

    VULKAN_HPP_INLINE VULKAN_HPP_CONSTEXPR
        PipelineFeatureFlags operator&( PipelineFeatureFlagBits bit0,
                                        PipelineFeatureFlagBits bit1)VULKAN_HPP_NOEXCEPT
    {
        return PipelineFeatureFlags( bit0 ) & bit1;
    }

    VULKAN_HPP_INLINE VULKAN_HPP_CONSTEXPR
        PipelineFeatureFlags operator^( PipelineFeatureFlagBits bit0,
                                        PipelineFeatureFlagBits bit1 ) VULKAN_HPP_NOEXCEPT
    {
        return PipelineFeatureFlags( bit0 ) ^ bit1;
    }

    VULKAN_HPP_INLINE VULKAN_HPP_CONSTEXPR
        PipelineFeatureFlags operator~( PipelineFeatureFlagBits bits ) VULKAN_HPP_NOEXCEPT
    {
        return ~( PipelineFeatureFlags( bits ) );
    }
} // namespace trc
