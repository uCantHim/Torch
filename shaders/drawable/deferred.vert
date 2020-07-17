#version 460
#extension GL_GOOGLE_include_directive : require

layout (location = 0) in vec3 vertexPosition;
layout (location = 1) in vec3 vertexNormal;
layout (location = 2) in vec2 vertexUv;
layout (location = 3) in vec3 vertexTangent;

layout (set = 0, binding = 0, std140) uniform CameraBuffer
{
    mat4 viewMatrix;
    mat4 projMatrix;
    mat4 inverseViewMatrix;
    mat4 inverseProjMatrix;
} camera;

layout (push_constant) uniform PushConstants
{
    mat4 modelMatrix;
    uint materialIndex;
};

layout (location = 0) out Vertex
{
    vec3 worldPos;
    vec3 normal;
    vec2 uv;
    uint material;
} vert;


/////////////////////
//      Main       //
/////////////////////

void main()
{
    vec4 worldPos = modelMatrix * vec4(vertexPosition, 1.0);
    gl_Position = camera.projMatrix * camera.viewMatrix * worldPos;

    vert.worldPos = worldPos.xyz;
    vert.normal = (transpose(inverse(modelMatrix)) * vec4(vertexNormal, 0.0)).xyz;
    vert.uv = vertexUv;
    vert.material = materialIndex;
}
