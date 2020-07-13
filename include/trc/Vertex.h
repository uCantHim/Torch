#pragma once

#include "Boilerplate.h"

namespace trc
{
    struct Vertex
    {
        vec3 position;
        vec3 normal;
        vec2 uv;
        vec3 tangent;
    };

    using VertexIndex = ui32;
} // namespace trc
