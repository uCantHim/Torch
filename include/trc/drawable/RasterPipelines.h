#pragma once

#include "Types.h"

#include "core/Pipeline.h"

namespace trc
{
    enum class PipelineFeatureFlagBits : ui64
    {
        eTransparent = 1 << 0,
        eAnimated    = 1 << 1,
        ePickable    = 1 << 2,

        eShadow      = 1 << 3,
    };

    using PipelineFeatureFlags = vk::Flags<PipelineFeatureFlagBits>;

    auto getPipeline(PipelineFeatureFlags featureFlags) -> Pipeline::ID;

    auto getDrawableDeferredPipeline() -> Pipeline::ID;
    auto getDrawableDeferredAnimatedPipeline() -> Pipeline::ID;
    auto getDrawableDeferredPickablePipeline() -> Pipeline::ID;
    auto getDrawableDeferredAnimatedAndPickablePipeline() -> Pipeline::ID;

    auto getDrawableTransparentDeferredPipeline() -> Pipeline::ID;
    auto getDrawableTransparentDeferredAnimatedPipeline() -> Pipeline::ID;
    auto getDrawableTransparentDeferredPickablePipeline() -> Pipeline::ID;
    auto getDrawableTransparentDeferredAnimatedAndPickablePipeline() -> Pipeline::ID;

    auto getDrawableShadowPipeline() -> Pipeline::ID;
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
                   | VkFlags(trc::PipelineFeatureFlagBits::ePickable)
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
