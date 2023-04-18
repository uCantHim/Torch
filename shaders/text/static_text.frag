#version 460
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require

#define ASSET_DESCRIPTOR_SET_BINDING 1
#include "asset_registry_descriptor.glsl"

#define TRANSPARENCY_SET_INDEX 2
#include "transparency.glsl"

layout (early_fragment_tests) in;

layout (location = 0) in VertexData {
    sample vec2 uv;
    flat uint textureIndex;
} vert;

void main()
{
    const float alpha = smoothstep(0.0, 1.0, texture(glyphTextures[vert.textureIndex], vert.uv).r);
    if (alpha < 0.01) {
        discard;
    }

    appendFragment(vec4(1.0, 1.0, 1.0, alpha));
}
