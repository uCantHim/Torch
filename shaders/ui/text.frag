#version 460
#extension GL_EXT_nonuniform_qualifier : require

layout (set = 0, binding = 0) uniform sampler2D fonts[];

layout (location = 0) in Vertex {
    vec2 uv;
    flat uint fontIndex;
    vec4 color;
} vert;

layout (location = 0) out vec4 fragColor;

#define TEXT_BOLDNESS 1.3

void main()
{
    float alpha = texture(fonts[vert.fontIndex], vert.uv).r * TEXT_BOLDNESS;
    fragColor = vec4(vert.color.rgb, vert.color.a * alpha);
}
