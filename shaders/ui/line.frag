#version 460

layout (push_constant) uniform PushConstants {
    vec2 start;
    vec2 end;
    vec4 color;
};

layout (location = 0) out vec4 fragColor;

void main()
{
    fragColor = color;
}
