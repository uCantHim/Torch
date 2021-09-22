#version 460
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require

#define ASSET_DESCRIPTOR_SET_BINDING 1
#include "../asset_registry_descriptor.glsl"

#define LIGHT_DESCRIPTOR_SET 2
#define LIGHT_DESCRIPTOR_BINDING 0
#define SHADOW_DESCRIPTOR_SET_BINDING 5
#include "../lighting.glsl"

#define TRANSPARENCY_SET_INDEX 3
#include "../transparency.glsl"

// Compare early against the depth values from the opaque pass
layout (early_fragment_tests) in;

// Constants
layout (constant_id = 1) const bool isPickable = false;

// Buffers
layout (set = 0, binding = 0, std140) restrict uniform CameraBuffer
{
    mat4 viewMatrix;
    mat4 projMatrix;
    mat4 inverseViewMatrix;
    mat4 inverseProjMatrix;
} camera;

layout (set = 0, binding = 1) restrict readonly uniform GlobalDataBuffer
{
    vec2 mousePos;
    vec2 resolution;
} global;

layout (set = 2, binding = 1) restrict buffer PickingBuffer
{
    uint pickableID;
    uint instanceID;
    float depth;
} picking;

// Push Constants
layout (push_constant) uniform PushConstants
{
    layout (offset = 84) uint pickableID;
};

// Input
layout (location = 0) in VertexData
{
    vec3 worldPos;
    vec2 uv;
    flat uint material;
    mat3 tbn;

    flat uint instanceIndex;
} vert;


/////////////////////
//      Main       //
/////////////////////

vec3 calcVertexNormal();

void main()
{
    uint diffTex = materials[vert.material].diffuseTexture;
    vec4 diffuseColor = materials[vert.material].color;
    if (diffTex != NO_TEXTURE) {
        diffuseColor = texture(textures[diffTex], vert.uv);
    }

    vec3 color = calcLighting(
        diffuseColor.rgb,
        vert.worldPos,
        calcVertexNormal(),
        camera.inverseViewMatrix[3].xyz,
        vert.material
    );

    if (diffuseColor.a > 0.0) {
        appendFragment(vec4(color, diffuseColor.a));
    }

    if (isPickable) // constant
    {
        // Floor because Vulkan sets the frag coord to .5 (the middle of the pixel)
        if (floor(gl_FragCoord.xy) == global.mousePos
            && gl_FragCoord.z < picking.depth)
        {
            picking.pickableID = pickableID;
            picking.instanceID = vert.instanceIndex;
            picking.depth = gl_FragCoord.z;
        }
    }
}


vec3 calcVertexNormal()
{
    uint bumpTex = materials[vert.material].bumpTexture;
    if (bumpTex == NO_TEXTURE)
    {
        return normalize(vert.tbn[2]);
    }
    else
    {
        vec3 textureNormal = texture(textures[bumpTex], vert.uv).rgb * 2.0 - 1.0;
        textureNormal.y = -textureNormal.y;  // Vulkan axis flip
        return normalize(vert.tbn * textureNormal);
    }
}
