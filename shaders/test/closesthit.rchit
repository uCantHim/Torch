#version 460
#extension GL_EXT_ray_tracing : require

layout (location = 0) rayPayloadInEXT vec4 color;

void main()
{
    // Hit is green
    color = vec4(0, 1, 0, 1);
}
