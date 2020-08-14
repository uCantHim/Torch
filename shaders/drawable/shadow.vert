#version 460
#extension GL_GOOGLE_include_directive : require

layout (location = 0) in vec3 vertexPosition;
layout (location = 1) in vec3 vertexNormal;
layout (location = 2) in vec2 vertexUv;
layout (location = 3) in vec3 vertexTangent;

layout (set = 0, binding = 0) buffer ShadowMatrices
{
    // These are the view+proj matrices
    mat4 shadowMatrices[];
};

layout (push_constant) uniform PushConstants
{
    mat4 modelMatrix;
    uint lightIndex;
    bool isAnimated;
} pushConstants;

void main()
{
    gl_Position = pushConstants.modelMatrix * vec4(vertexPosition, 1.0);
}
