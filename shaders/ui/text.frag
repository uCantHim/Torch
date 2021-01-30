#version 460
#extension GL_EXT_nonuniform_qualifier : require

layout (set = 0, binding = 0) uniform sampler2D fonts[];

layout (location = 0) in Vertex {
    vec2 uv;
    flat uint fontIndex;
} vert;

layout (location = 0) out vec4 fragColor;

void main()
{
    float alpha = texture(fonts[vert.fontIndex], vert.uv).r;
    alpha = smoothstep(0.0, 1.0, alpha);
    //alpha = step(0.1, alpha);
    fragColor = vec4(1, 1, 1, alpha);
}
