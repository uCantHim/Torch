#version 460
#extension GL_EXT_nonuniform_qualifier : require

#define ASSET_DESCRIPTOR_SET_BINDING 1
#include "asset_registry_descriptor.glsl"

#define TRANSPARENCY_SET_INDEX 2
#include "transparency.glsl"

layout (early_fragment_tests) in;

///////////////////
//      Main     //
///////////////////

layout (location = 0) in VertexData
{
    vec2 uv;
    vec3 normal;  // unused
    flat uint textureIndex;
} vert;

void main()
{
    vec4 diffuseColor = texture(textures[vert.textureIndex], vert.uv);
    if (diffuseColor.a > 0.001) {
        appendFragment(diffuseColor);
    }
}
