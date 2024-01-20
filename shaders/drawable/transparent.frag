#version 460
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require

#define ASSET_DESCRIPTOR_SET_BINDING 1
#include "asset_registry_descriptor.glsl"

#define LIGHT_DESCRIPTOR_SET 2
#define LIGHT_DESCRIPTOR_BINDING 0
#define SHADOW_DESCRIPTOR_SET_BINDING 4
#include "lighting.glsl"

#define TRANSPARENCY_SET_INDEX 3
#include "transparency.glsl"

// Compare early against the depth values from the opaque pass
layout (early_fragment_tests) in;

// Buffers
layout (set = 0, binding = 0, std140) restrict uniform CameraBuffer
{
    mat4 viewMatrix;
    mat4 projMatrix;
    mat4 inverseViewMatrix;
    mat4 inverseProjMatrix;
} camera;

// Input
layout (location = 0) in VertexData
{
    vec3 worldPos;
    vec2 uv;
    flat uint material;
    mat3 tbn;
} vert;


/////////////////////
//      Main       //
/////////////////////

vec3 calcVertexNormal();

void main()
{
    const uint diffTex = materials[vert.material].diffuseTexture;
    vec4 diffuseColor = materials[vert.material].color;
    if (diffTex != NO_TEXTURE) {
        diffuseColor = texture(textures[diffTex], vert.uv);
    }

    if (diffuseColor.a > 0.0 && materials[vert.material].performLighting)
    {
        MaterialParams mat;
        mat.kSpecular = materials[vert.material].kSpecular;
        mat.roughness = materials[vert.material].roughness;
        mat.metallicness = materials[vert.material].metallicness;

        vec3 color = calcLighting(
            diffuseColor.rgb,
            vert.worldPos,
            calcVertexNormal(),
            camera.inverseViewMatrix[3].xyz,
            mat
        );

        appendFragment(vec4(color, diffuseColor.a));
    }
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
