#version 460

layout (location = 0) out vec4 fragColor;

layout (location = 0) in Vertex
{
    vec3 normal;
    vec2 uv;
} vert;

const vec3 LIGHT_DIR = vec3(1, 1, -1);

void main()
{
    vec3 color = vec3(0.0, 1.0, 1.0);
    color *= max(0.0, dot(vert.normal, normalize(LIGHT_DIR)));

    fragColor = vec4(color, 1.0);
}
