// Descriptor binding declarations for the asset descriptor

#include "material.glsl"
#include "vertex.glsl"

layout (set = ASSET_DESCRIPTOR_SET_BINDING, binding = 0, std430) restrict readonly buffer Materials
{
    Material materials[];
};

layout (set = ASSET_DESCRIPTOR_SET_BINDING, binding = 1) uniform sampler2D textures[];

layout (set = ASSET_DESCRIPTOR_SET_BINDING, binding = 2, std430) restrict readonly buffer VertexBuffers
{
    Vertex vertices[];
} vertexBuffers[];

layout (set = ASSET_DESCRIPTOR_SET_BINDING, binding = 3, std430) restrict readonly buffer IndexBuffers
{
    uint indices[];
} indexBuffers[];
