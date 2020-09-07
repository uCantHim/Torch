#version 460

layout (location = 0) in vec3 vertexPosition;
layout (location = 1) in vec2 vertexUv;
layout (location = 2) in vec3 vertexNormal;
layout (location = 3) in mat4 modelMatrix;

layout (set = 0, binding = 0, std140) buffer ShadowMatrices
{
    mat4 shadowMatrices[];
};

layout (set = 1, binding = 0, std140) uniform CameraBuffer
{
    mat4 viewMatrix;
    mat4 projMatrix;
    mat4 inverseViewMatrix;
    mat4 inverseProjMatrix;
} camera;

layout (push_constant) uniform PushConstants
{
    uint currentLight;
} pushConstants;

void main()
{
    const mat4 viewProjMat = shadowMatrices[pushConstants.currentLight];
    mat4 viewInverseRotation = camera.inverseViewMatrix;
    viewInverseRotation[3] = vec4(0, 0, 0, 1);

    gl_Position = viewProjMat * viewInverseRotation * modelMatrix * vec4(vertexPosition, 1.0);
}
