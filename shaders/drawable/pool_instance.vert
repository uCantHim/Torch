#version 460
#extension GL_GOOGLE_include_directive : require

#define BONE_INDICES_INPUT_LOCATION 4
#define BONE_WEIGHTS_INPUT_LOCATION 5
#define ANIM_DESCRIPTOR_SET_BINDING 4
#include "../animation.glsl"

layout (constant_id = 0) const bool isAnimated = false;

layout (location = 0) in vec3 vertexPosition;
layout (location = 1) in vec3 vertexNormal;
layout (location = 2) in vec2 vertexUv;
layout (location = 3) in vec3 vertexTangent;

// layout boneIndices
// layout boneWeights

layout (location = 6) in mat4 instanceModelMatrix;
layout (location = 10) in uvec4 instanceAnimData;

layout (set = 0, binding = 0, std140) uniform CameraBuffer
{
    mat4 viewMatrix;
    mat4 projMatrix;
    mat4 inverseViewMatrix;
    mat4 inverseProjMatrix;
} camera;

layout (push_constant) uniform PushConstants
{
    uint materialIndex;
};

layout (location = 0) out Vertex
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

    const uint animation = instanceAnimData[0];
    if (animation != NO_ANIMATION)
    {
        const uint keyframes[2] = { instanceAnimData[1], instanceAnimData[2] };
        const float weight = uintBitsToFloat(instanceAnimData[3]);
        vertPos = applyAnimation(animation, vertPos, keyframes, weight);
        normal = applyAnimation(animation, normal, keyframes, weight);
        tangent = applyAnimation(animation, tangent, keyframes, weight);
    }
    vertPos.w = 1.0;

    vec4 worldPos = instanceModelMatrix * vertPos;
    gl_Position = camera.projMatrix * camera.viewMatrix * worldPos;

    vert.worldPos = worldPos.xyz;
    vert.uv = vertexUv;
    vert.material = materialIndex;

    vec3 N = normalize((transpose(inverse(instanceModelMatrix)) * normal).xyz);
    vec3 T = normalize((instanceModelMatrix * tangent).xyz);
    vec3 B = cross(N, T);
    vert.tbn = mat3(T, B, N);
}
