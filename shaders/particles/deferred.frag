#version 460
#extension GL_EXT_nonuniform_qualifier : require

#define TRANSPARENCY_SET_INDEX 2
#include "../transparency.glsl"

layout (early_fragment_tests) in;

layout (set = 1, binding = 1) uniform sampler2D textures[];

///////////////////
//      Main     //
///////////////////

layout (location = 0) in Vertex
{
    vec2 uv;
    flat uint textureIndex;
} vert;

void main()
{
    vec4 diffuseColor = texture(textures[vert.textureIndex], vert.uv);
    if (diffuseColor.a > 0.001) {
        appendFragment(diffuseColor);
    }
}
