#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (set = 0, binding = 1) uniform sampler2D tex;

layout (location = 0) out vec4 fragColor;

in Vertex {
    layout(location = 0) in vec3 position;
    layout(location = 1) in vec3 normal;
    layout(location = 2) in vec2 uvCoords;
} vert;

void main()
{
    fragColor = texture(tex, vert.uvCoords);
}
