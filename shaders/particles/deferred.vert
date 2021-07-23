#version 460

layout (location = 0) in vec3 vertexPosition;
layout (location = 1) in vec2 vertexUv;
layout (location = 2) in vec3 vertexNormal;
layout (location = 3) in mat4 modelMatrix;

// Material properties
layout (location = 7) in uint textureIndex;

layout (set = 0, binding = 0, std140) restrict readonly uniform CameraBuffer
{
    mat4 viewMatrix;
    mat4 projMatrix;
    mat4 inverseViewMatrix;
    mat4 inverseProjMatrix;
} camera;

layout (location = 0) out Vertex
{
    vec2 uv;
    flat uint textureIndex;
} vert;

void main()
{
    // Apply the inverse view rotation to the particle to orient it towards the camera.
    vec4 worldPos = camera.inverseViewMatrix * modelMatrix * vec4(vertexPosition, 1.0);
    worldPos.xyz -= camera.inverseViewMatrix[3].xyz;

    gl_Position = camera.projMatrix * camera.viewMatrix * worldPos;
    vert.uv = vertexUv;
    vert.textureIndex = textureIndex;
}
