#version 460
#extension GL_GOOGLE_include_directive : require

#define BONE_INDICES_INPUT_LOCATION 4
#define BONE_WEIGHTS_INPUT_LOCATION 5
#define ASSET_DESCRIPTOR_SET_BINDING 1
#include "animation.glsl"

layout (location = 0) in vec3 vertexPosition;
layout (location = 1) in vec3 vertexNormal;
layout (location = 2) in vec2 vertexUv;
layout (location = 3) in vec3 vertexTangent;

layout (set = 0, binding = 0, std140) restrict uniform CameraBuffer
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

    AnimationPushConstantData animData;
};

layout (location = 0) out VertexData
{
    vec3 worldPos;
    vec2 uv;
    flat uint material;
    mat3 tbn;
} vert;


/////////////////////
//      Main       //
/////////////////////

void main()
{
    vec4 vertPos = vec4(vertexPosition, 1.0);
    vec4 normal = vec4(vertexNormal, 0.0);
    vec4 tangent = vec4(vertexTangent, 0.0);

    vertPos = applyAnimation(animData.animation, vertPos, animData.keyframes, animData.keyframeWeigth);
    normal = applyAnimation(animData.animation, normal, animData.keyframes, animData.keyframeWeigth);
    tangent = applyAnimation(animData.animation, tangent, animData.keyframes, animData.keyframeWeigth);
    vertPos.w = 1.0;

    vec4 worldPos = modelMatrix * vertPos;
    gl_Position = camera.projMatrix * camera.viewMatrix * worldPos;

    // Set out-attributes
    vert.worldPos = worldPos.xyz;
    vert.uv = vertexUv;
    vert.material = materialIndex;

    vec3 N = normalize((transpose(inverse(modelMatrix)) * normal).xyz);
    vec3 T = normalize((modelMatrix * tangent).xyz);
    vec3 B = cross(N, T);
    vert.tbn = mat3(T, B, N);
}
