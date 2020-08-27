#version 460
#extension GL_GOOGLE_include_directive : require

#include "../animation.glsl"

layout (location = 0) in vec3 vertexPosition;
layout (location = 1) in vec3 vertexNormal;
layout (location = 2) in vec2 vertexUv;
layout (location = 3) in vec3 vertexTangent;

layout (location = 6) in mat4 instanceModelMatrix;

layout (set = 0, binding = 0, std430) buffer ShadowMatrices
{
    // These are the view-proj matrices
    mat4 shadowMatrices[];
};

layout (push_constant) uniform PushConstants
{
    uint lightIndex;  // Index into shadow matrix buffer
};

void main()
{
    mat4 viewProj = shadowMatrices[lightIndex];
    vec4 vertPos = vec4(vertexPosition, 1.0);

    gl_Position = viewProj * instanceModelMatrix * vertPos;
}
