#pragma once

#include "trc/Types.h"

namespace trc
{
    using VertexIndex = ui32;

    struct MeshVertex
    {
        vec3 position{ 0, 0, 0 };
        vec3 normal{ 0, 0, 0 };
        vec2 uv{ 0, 0 };
        vec3 tangent{ 0, 0, 0 };
    };

    struct SkeletalVertex
    {
        uvec4 boneIndices{ UINT32_MAX };
        vec4 boneWeights{ 0.0f };
    };
} // namespace trc
