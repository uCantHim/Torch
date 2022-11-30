#pragma once

#include "ShaderCapabilities.h"
#include "ShaderModuleBuilder.h"

namespace trc
{
    struct FragmentCapability
    {
        static constexpr Capability kVertexWorldPos{ "frag_vertexWorldPos" };
        static constexpr Capability kVertexNormal{ "frag_vertexNormal" };
        static constexpr Capability kVertexUV{ "frag_vertexUV" };
        static constexpr Capability kTangentToWorldSpaceMatrix{ "frag_tangentToWorld" };

        static constexpr Capability kCameraWorldPos{ "frag_cameraWorldPos" };

        static constexpr Capability kTime{ "frag_currentTime" };
        static constexpr Capability kTimeDelta{ "frag_frameTime" };

        static constexpr Capability kTextureSample{ "frag_textureSample" };

        static constexpr Capability kNextFragmentListIndex{ "frag_allocFragListIndex" };
        static constexpr Capability kMaxFragmentListIndex{ "frag_maxFragListIndex" };
        static constexpr Capability kFragmentListHeadPointerImage{ "frag_fragListPointerImage" };
        static constexpr Capability kFragmentListBuffer{ "frag_fragListBuffer" };
        static constexpr Capability kShadowMatrices{ "frag_shadowMatrixBuffer" };
        static constexpr Capability kLightBuffer{ "frag_lightDataBuffer" };
    };
} // namespace trc
