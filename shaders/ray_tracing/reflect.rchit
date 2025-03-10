#version 460
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier: require
#extension GL_EXT_ray_tracing : require

#define ASSET_DESCRIPTOR_SET_BINDING 3
#define LIGHT_DESCRIPTOR_SET 4
#define LIGHT_DESCRIPTOR_BINDING 0
#define SHADOW_DESCRIPTOR_SET_BINDING 5
#include "asset_registry_descriptor.glsl"
#include "lighting.glsl"
#include "ray_tracing/hit_utils.glsl"

struct DrawableData
{
    uint geo;
    uint mat;
};

layout (set = 4, binding = 1, std430) restrict readonly buffer DrawableDataBuffer
{
    DrawableData drawables[];
};

hitAttributeEXT vec2 barycentricCoords;
layout (location = 0) rayPayloadInEXT vec3 color;

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
    const vec3 normal = calcHitNormal(baryCoords, geo, gl_PrimitiveID);
    const vec2 uv = calcHitUv(baryCoords, geo, gl_PrimitiveID);

    const uint sbtRecordIndex = mat;

    /**
     * Perform calculations of material parameters here
     */
    vec3 albedo = vec3(1);

    MaterialParams params;
    params.kSpecular = 1.0f;
    params.roughness = 1.0f;
    params.metallicness = 0.0f;
    /** */

    color = calcLighting(
        albedo,
        hitPos,
        normal,
        gl_WorldRayOriginEXT,
        params
    );
}
