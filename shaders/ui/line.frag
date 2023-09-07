#version 460

layout (push_constant) uniform PushConstants {
    layout (offset=16) vec4 color;
};

layout (location = 0) out vec4 fragColor;

void main()
{
    fragColor = color;
}
