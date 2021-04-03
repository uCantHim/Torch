#version 460

layout (location = 0) in vec2 vertexUv;

layout (push_constant) uniform PushConstants {
    vec2 start;
    vec2 end;
};

void main()
{
    const vec2 pos = start + (end - start) * vertexUv;
    gl_Position = vec4(pos * 2.0 - 1.0, 0.0, 1.0);
}
