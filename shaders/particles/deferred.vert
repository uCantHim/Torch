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
    const vec4 worldPos = modelMatrix * vec4(vertexPosition, 1.0);

    // Don't apply camera rotation - the particles always face the camera
    gl_Position = camera.projMatrix * vec4((camera.viewMatrix[3].xyz + worldPos.xyz), 1.0);

    vert.worldPos = worldPos.xyz;
    vert.uv = vertexUv;
    vert.normal = (transpose(inverse(modelMatrix)) * vec4(vertexNormal, 0.0)).xyz;
}
