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

        uvec4 boneIndices{ UINT32_MAX };
        vec4 boneWeights{ 0.0f };
    };

    static inline auto makeVertexAttributeDescriptions()
        -> std::vector<vk::VertexInputAttributeDescription>
    {
        return {
            vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32B32Sfloat, 0),
            vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32Sfloat, 12),
            vk::VertexInputAttributeDescription(2, 0, vk::Format::eR32G32Sfloat, 24),
            vk::VertexInputAttributeDescription(3, 0, vk::Format::eR32G32B32Sfloat, 32),
        };
    }
} // namespace trc
