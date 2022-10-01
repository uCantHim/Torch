#version 460
#extension GL_EXT_ray_tracing : require

layout (location = 0) rayPayloadInEXT vec4 color;

hitAttributeEXT vec2 bary;

void main()
{
    // Hit is green
    color = vec4(bary.x, bary.y, 1.0 - bary.x - bary.y, 1);
}
