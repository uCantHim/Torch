#version 460
#extension GL_GOOGLE_include_directive : require

layout (location = 0) in vec3 vertexPosition;

layout (set = 0, binding = 0, std430) buffer ShadowMatrices
{
    // These are the view-proj matrices
    mat4 shadowMatrices[];
};

layout (push_constant) uniform PushConstants
{
    mat4 modelMatrix;
    uint shadowIndex;  // Index into shadow matrix buffer
};

void main()
{
    mat4 viewProj = shadowMatrices[shadowIndex];
    vec4 vertPos = vec4(vertexPosition, 1.0);

    gl_Position = viewProj * modelMatrix * vertPos;
}
