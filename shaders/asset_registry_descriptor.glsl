// Descriptor binding declarations for the asset descriptor

#include "material.glsl"
#include "vertex.glsl"

struct AnimationMetaData
{
    uint baseOffset;
    uint frameCount;
    uint boneCount;
};

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

layout (set = ASSET_DESCRIPTOR_SET_BINDING, binding = 4, std430) restrict readonly buffer AnimationMeta
{
    // Indexed by animation indices provided by the client
    AnimationMetaData metas[];
} animMeta;

layout (set = ASSET_DESCRIPTOR_SET_BINDING, binding = 5, std140) restrict readonly buffer Animation
{
    // Animations start at offsets defined in the AnimationMetaData array
    mat4 boneMatrices[];
} animations;
