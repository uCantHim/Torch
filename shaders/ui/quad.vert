#version 460

layout (location = 0) in vec2 vertexPosition;

layout (push_constant) uniform PushConstants {
    vec2 pos;
    vec2 size;
    vec4 color;
} push;

layout (location = 0) out Vertex {
    vec4 color;
};

void main()
{
    vec2 pos = (push.size * vertexPosition) + push.pos;
    gl_Position = vec4(pos * 2.0 - 1.0, 0.0, 1.0);
    color = push.color;
}
