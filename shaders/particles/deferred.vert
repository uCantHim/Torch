#version 460

layout (location = 0) in vec3 vertexPosition;
layout (location = 1) in vec2 vertexUv;
layout (location = 2) in vec3 vertexNormal;
layout (location = 3) in mat4 modelMatrix;
// layout (location = 5) in uint isEmitting;
// layout (location = 6) in uint hasShadow;
// layout (location = 7) in uint textureIndex;

layout (set = 0, binding = 0, std140) restrict readonly uniform CameraBuffer
{
    mat4 viewMatrix;
    mat4 projMatrix;
    mat4 inverseViewMatrix;
    mat4 inverseProjMatrix;
} camera;

layout (location = 0) out Vertex
{
    vec3 worldPos;
    vec2 uv;
    vec3 normal;
} vert;

void main()
{
    mat4 viewInverseRotation = camera.inverseViewMatrix;
    viewInverseRotation[3] = vec4(0, 0, 0, 1);

    // Apply the inverse view rotation to the particle to orient it towards the camera.
    // Add this to the world position because the rotation has to be visible in the
    // final lighting calculation.
    const vec4 worldPos = viewInverseRotation * modelMatrix * vec4(vertexPosition, 1.0);

    gl_Position = camera.projMatrix * camera.viewMatrix * worldPos;
    vert.worldPos = worldPos.xyz;
    vert.uv = vertexUv;
    vert.normal = (transpose(inverse(modelMatrix)) * vec4(vertexNormal, 0.0)).xyz;
}
