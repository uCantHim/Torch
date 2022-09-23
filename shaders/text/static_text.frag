#version 460

#define TRANSPARENCY_SET_INDEX 2
#include "transparency.glsl"

layout (early_fragment_tests) in;

layout (set = 1, binding = 0) uniform sampler2D glyphTexture;

layout (location = 0) in Vertex {
    sample vec2 uv;
} vert;

void main()
{
    const float alpha = smoothstep(0.0, 1.0, texture(glyphTexture, vert.uv).r);
    if (alpha < 0.01) {
        discard;
    }

    appendFragment(vec4(1.0, 1.0, 1.0, alpha));
}
