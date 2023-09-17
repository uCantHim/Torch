#version 460

layout (set = 1, binding = 0) uniform sampler2D image;

layout (location = 0) in PerVertex
{
    vec2 uv;
    vec4 color;
} vert;

layout (location = 0) out vec4 fragColor;

void main()
{
    fragColor = vert.color * texture(image, vert.uv).r;
}
