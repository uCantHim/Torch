#version 460
#extension GL_GOOGLE_include_directive : require

#include "../resources.glsl"

layout (location = 0) in vec3 vertexPosition;
layout (location = 1) in vec3 vertexNormal;
layout (location = 2) in vec2 vertexUv;
layout (location = 3) in vec3 vertexTangent;

//layout (set = 0, binding = 0) uniform CameraBuffer
//{
//    mat4 viewMatrix;
//    mat4 projMatrix;
//    mat4 inverseViewMatrix;
//    mat4 inverseProjMatrix;
//} camera;

layout (set = 0, binding = 0, std140) buffer readonly MaterialBuffer
{
    Material materials[];
};

layout (push_constant) uniform PushConstants
{
    mat4 modelMatrix;
    uint materialIndex;
};

layout (location = 0) out Vertex
{
    vec3 normal;
    vec2 uv;
} vert;


/////////////////////
//      Main       //
/////////////////////

void main()
{
    gl_Position = /* camera.projMatrix * camera.viewMatrix * */ modelMatrix * vec4(vertexPosition, 1.0);
    vert.normal = normalize((transpose(inverse(modelMatrix)) * vec4(vertexNormal, 0.0)).xyz);
    vert.uv = vertexUv;
}
