#version 460
#extension GL_EXT_ray_tracing : require

layout (location = 0) rayPayloadInEXT vec4 color;

void main()
{
    color = vec4(1, 0.6, 0, 1);
}
