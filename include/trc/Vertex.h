#pragma once

#include "trc/Types.h"

namespace trc
{
    using VertexIndex = ui32;

    struct MeshVertex
    {
        MeshVertex() = default;
        MeshVertex(vec3 p, vec3 n, vec2 uv, vec3 t)
            : position(p), normal(n), uv(uv), tangent(t) {}

        vec3 position;
        vec3 normal;
        vec2 uv;
        vec3 tangent;
    };

    struct SkeletalVertex
    {
        uvec4 boneIndices{ UINT32_MAX };
        vec4 boneWeights{ 0.0f };
    };
} // namespace trc
