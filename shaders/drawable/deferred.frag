#version 460
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require

#include "../material.glsl"

layout (early_fragment_tests) in;

// Buffers
layout (set = 0, binding = 1) restrict readonly uniform GlobalDataBuffer
{
    vec2 mousePos;
    vec2 resolution;
} global;

layout (set = 1, binding = 0, std430) restrict readonly buffer MaterialBuffer
{
    Material materials[];
};

layout (set = 1, binding = 1) uniform sampler2D textures[];

layout (set = 3, binding = 4, r32ui) uniform uimage2D fragmentListHeadPointer;

layout (set = 3, binding = 5) restrict buffer FragmentListAllocator
{
    uint nextFragmentListIndex;
    uint maxFragmentListIndex;
};

layout (set = 3, binding = 6) restrict buffer FragmentList
{
    /**
     * 0: A packed color
     * 1: Fragment depth value
     * 2: Next-pointer
     */
    uint fragmentList[][3];
};

// Input
layout (location = 0) in VertexData
{
    vec3 worldPos;
    vec2 uv;
    flat uint material;
    mat3 tbn;
} vert;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out uint outAlbedo;
layout (location = 2) out uint outMaterial;


/////////////////////
//      Main       //
/////////////////////

vec3 calcVertexColor();
vec3 calcVertexNormal();

void main()
{
    outNormal = calcVertexNormal();
    outAlbedo = packUnorm4x8(vec4(calcVertexColor(), 0.0));
    outMaterial = vert.material;
}


vec3 calcVertexColor()
{
    vec3 color = materials[vert.material].color.rgb;

    // Use diffuse texture if available
    uint diffTexture = materials[vert.material].diffuseTexture;
    if (diffTexture != NO_TEXTURE) {
        color = texture(textures[diffTexture], vert.uv).rgb;
    }

    return color;
}


vec3 calcVertexNormal()
{
    uint bumpTex = materials[vert.material].bumpTexture;
    if (bumpTex == NO_TEXTURE)
    {
        return normalize(vert.tbn[2]);
    }
    else
    {
        vec3 textureNormal = texture(textures[bumpTex], vert.uv).rgb * 2.0 - 1.0;
        textureNormal.y = -textureNormal.y;  // Vulkan axis flip
        return normalize(vert.tbn * textureNormal);
    }
}
