#version 460
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier: require
#extension GL_EXT_ray_tracing : require

#define ASSET_DESCRIPTOR_SET_BINDING 2
#define LIGHT_DESCRIPTOR_SET 3
#define LIGHT_DESCRIPTOR_BINDING 0
#include "../asset_registry_descriptor.glsl"
#include "hit_utils.glsl"
#include "../lighting.glsl"

struct DrawableData
{
    uint geo;
    uint mat;
};

layout (set = 0, binding = 1, std430) restrict readonly buffer DrawableDataBuffer
{
    DrawableData drawables[];
};

hitAttributeEXT vec2 barycentricCoords;
layout (location = 0) rayPayloadInEXT vec4 color;

void main()
{
    const vec3 baryCoords = vec3(
        1.0 - barycentricCoords.x - barycentricCoords.y,
        barycentricCoords.x,
        barycentricCoords.y
    );

    const uint drawableId = gl_InstanceCustomIndexEXT;
    const uint geo = drawables[drawableId].geo;
    const uint mat = drawables[drawableId].mat;

    const vec3 hitPos = gl_WorldRayOriginEXT + gl_HitTEXT * gl_WorldRayDirectionEXT;
    const vec3 normal = calcHitNormal(baryCoords, geo, mat);
    const vec2 uv = calcHitUv(baryCoords, geo);

    vec3 albedo = materials[mat].color.rgb;
    // Use diffuse texture if available
    uint diffTexture = materials[mat].diffuseTexture;
    if (diffTexture != NO_TEXTURE) {
        albedo = texture(textures[diffTexture], uv).rgb;
    }

    color.xyz = calcLighting(albedo, hitPos, normal, gl_WorldRayOriginEXT, mat);
}
