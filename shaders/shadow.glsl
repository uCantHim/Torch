// Functions that calculate shadows

#ifndef TRC_SHADOW_GLSL_INCLUDE
#define TRC_SHADOW_GLSL_INCLUDE

#extension GL_GOOGLE_include_directive : require

#ifndef SHADOW_DESCRIPTOR_SET_BINDING
#define SHADOW_DESCRIPTOR_SET_BINDING 4
#endif

#define NO_SHADOW 1.0
#define SHADOW_FACTOR_SUN_LIGHT 0.2

#include "light.glsl"

layout (set = SHADOW_DESCRIPTOR_SET_BINDING, binding = 0) restrict readonly buffer ShadowMatrixBuffer
{
    mat4 shadowMatrices[];
};

layout (set = SHADOW_DESCRIPTOR_SET_BINDING, binding = 1) uniform sampler2D shadowMaps[];

/**
 * @brief Tests if a point is in a shadow of a specific light
 */
bool isInShadow(vec3 worldCoords, uint shadowIndex)
{
    const float bias = 0.002;

    vec4 projCoords = shadowMatrices[shadowIndex] * vec4(worldCoords, 1.0);
    projCoords.xyz /= projCoords.w;

    const vec2 shadowMapUV = projCoords.xy * 0.5 + 0.5;
    const float objectDepth = projCoords.z; // Don't transform from [-1, 1] to [0, 1] because depth
                                            // is already in the range [0, 1] in Vulkan
    const float shadowDepth = texture(shadowMaps[shadowIndex], shadowMapUV).r;

    // Test if the world coordinates map to a point on the shadow map
    bool liesInShadowMap = shadowMapUV.x > 0.0 && shadowMapUV.x < 1.0
                        && shadowMapUV.y > 0.0 && shadowMapUV.y < 1.0;

    return (objectDepth > (shadowDepth + bias)) && liesInShadowMap;
}

/**
 * @brief Calculate the amount of light that reaches a point
 */
float calcLightShadowValueSharp(vec3 worldCoords, Light light)
{
    if (!light.hasShadow) {
        return NO_SHADOW;
    }

    switch (light.type)
    {
    case LIGHT_TYPE_SUN:
        if (isInShadow(worldCoords, light.firstShadowIndex)) {
            return SHADOW_FACTOR_SUN_LIGHT;
        }

        return NO_SHADOW;
    }

    return NO_SHADOW;
}

/**
 * @brief Calculate a shadow strength for a shadow map that is smooth at
 *        the shadow's edges.
 */
float calcSmoothShadowStrength(vec3 worldCoords, uint shadowIndex)
{
    const float bias = 0.002;

    vec4 projCoords = shadowMatrices[shadowIndex] * vec4(worldCoords, 1.0);
    projCoords.xyz /= projCoords.w;

    const vec2 shadowMapUV = projCoords.xy * 0.5 + 0.5;
    const float objectDepth = projCoords.z; // Don't transform from [-1, 1] to [0, 1] because depth
                                            // is already in the range [0, 1] in Vulkan

    // Exit early if the middle fragment isn't inside of the shadow map
    if (!(shadowMapUV.x > 0.0 && shadowMapUV.x < 1.0 && shadowMapUV.y > 0.0 && shadowMapUV.y < 1.0))
    {
        return 0.0;
    }

    const vec2 texelSize = vec2(1, 1) / vec2(textureSize(shadowMaps[shadowIndex], 0));

    float shadowStrength = 0.0;
    // A square field of nine samples
    for (int x = -1; x <= 1; x++)
    {
        for (int y = -1; y <= 1; y++)
        {
            const vec2 currentUV = shadowMapUV + vec2(x, y) * texelSize;
            const float shadowDepth = texture(shadowMaps[shadowIndex], currentUV).r;

            // Test if the world coordinates map to a point on the shadow map
            bool liesInShadowMap = currentUV.x > 0.0 && currentUV.x < 1.0
                                && currentUV.y > 0.0 && currentUV.y < 1.0;
            bool objectIsInShadow = objectDepth > (shadowDepth + bias);

            shadowStrength += 1.0 * float(objectIsInShadow && liesInShadowMap);
        }
    }

    return shadowStrength / 9.0;
}

float calcLightShadowValueSmooth(vec3 worldCoords, Light light)
{
    if (!light.hasShadow) {
        return NO_SHADOW;
    }

    switch (light.type)
    {
    case LIGHT_TYPE_SUN:
        return SHADOW_FACTOR_SUN_LIGHT
               + 0.8 * (1.0 - calcSmoothShadowStrength(worldCoords, light.firstShadowIndex));
    }

    return NO_SHADOW;
}

#endif
