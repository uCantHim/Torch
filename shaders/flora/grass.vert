#version 460

layout (location = 0) in vec2 vertexUv;
layout (location = 1) in vec3 instancePosition;
layout (location = 2) in vec2 instanceUvPos; // UV position on the texture map
layout (location = 3) in vec2 instanceUvSize; // UV size on the texture map

layout (set = 0, binding = 0, std140) restrict uniform CameraBuffer
{
    mat4 viewMatrix;
    mat4 projMatrix;
    mat4 inverseViewMatrix;
    mat4 inverseProjMatrix;
} camera;

layout (push_constant) uniform PushConstants {
    vec3 cameraRight;
    vec3 cameraUp;
};

layout (location = 0) out Vertex
{
    vec4 pos;
    vec3 normal;
    vec2 uv;
    flat uint material;
} vert;

void main()
{
    vec2 signedUv = vertexUv * 2.0 - 1.0;
    vec3 vertexPos = signedUv.x * cameraRight + signedUv.y * cameraUp;
    vert.pos = vec4(instancePosition + vertexPos, 1.0);
    vert.normal = -cross(cameraUp, cameraRight);
    vert.uv = instanceUvPos + vertexUv * instanceUvSize;

    gl_Position = camera.projMatrix * camera.viewMatrix * vert.pos;
}
