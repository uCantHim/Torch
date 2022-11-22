#pragma once

#include "ShaderCapabilities.h"

namespace trc
{
    struct FragmentCapability
    {
        static constexpr Capability kVertexWorldPos{ "frag_vertexWorldPos" };
        static constexpr Capability kVertexNormal{ "frag_vertexNormal" };
        static constexpr Capability kVertexUV{ "frag_vertexUV" };
        static constexpr Capability kTangentToWorldSpaceMatrix{ "frag_tangentToWorld" };

        static constexpr Capability kTime{ "frag_currentTime" };
        static constexpr Capability kTimeDelta{ "frag_frameTime" };

        static constexpr Capability kTextureSample{ "frag_textureSample" };
    };
} // namespace trc
