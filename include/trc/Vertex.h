#pragma once

#include <glm/glm.hpp>
using namespace glm;

namespace trc
{
    struct Vertex
    {
        vec3 position;
        vec3 normal;
        vec2 uv;
        vec3 tangent;
    };

    using VertexIndex = uint32_t;
} // namespace trc
