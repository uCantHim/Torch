#version 460
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require

#define ASSET_DESCRIPTOR_SET_BINDING 1
#include "asset_registry_descriptor.glsl"

///////////////////
//      Main     //
///////////////////

layout (location = 0) in VertexData
{
    vec2 uv;
    vec3 normal;
    flat uint textureIndex;
} vert;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec4 outAlbedo;
layout (location = 2) out vec4 outMaterial;

void main()
{
    vec4 diffuseColor = texture(textures[vert.textureIndex], vert.uv);
    if (diffuseColor.a < 0.001) {
        discard;
    }

    outNormal = vert.normal;
    outAlbedo = diffuseColor;
    outMaterial = vec4(1.0f, 1.0f, 0.0f, 0.0f);
}
