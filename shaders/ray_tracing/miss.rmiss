#version 460
#extension GL_EXT_ray_tracing : require
//#extension GL_NV_ray_tracing : require

layout (location = 0) rayPayloadEXT vec4 color;

void main()
{
    // Miss is light blue
    color = vec4(0, 0.3, 1, 1);
}
