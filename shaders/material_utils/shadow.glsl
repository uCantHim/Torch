// Functions that calculate shadows

#ifndef TRC_SHADOW_GLSL_INCLUDE
#define TRC_SHADOW_GLSL_INCLUDE

#extension GL_GOOGLE_include_directive : require

#include "material_utils/light.glsl"

#define SHADOW_MATRICES $shadowMatrixBufferName

const float SHADOW_BIAS = 0.0003;
const float NO_SHADOW = 1.0;
const float SHADOW_FACTOR_SUN_LIGHT = 0.2;

/**
 * @brief Tests if a point is in a shadow on a specific shadow map
 *
 * @param vec3 worldCoords The point in world space to test for whether it
 *                         is in a shadow.
 * @param uint shadowIndex Index of the shadow map and its corresponding
 *                         shadow matrix
 */
bool isInShadow(vec3 worldCoords, uint shadowIndex)
{
    vec4 projCoords = SHADOW_MATRICES[shadowIndex] * vec4(worldCoords, 1.0);
    projCoords.xyz /= projCoords.w;

    const vec2 shadowMapUV = projCoords.xy * 0.5 + 0.5;
    const float objectDepth = projCoords.z; // Don't transform from [-1, 1] to [0, 1] because depth
                                            // is already in the range [0, 1] in Vulkan
    const float shadowDepth = texture(shadowMaps[shadowIndex], shadowMapUV).r;

    // Test if the world coordinates map to a point on the shadow map
    bool liesInShadowMap = shadowMapUV.x > 0.0 && shadowMapUV.x < 1.0
                        && shadowMapUV.y > 0.0 && shadowMapUV.y < 1.0
                        && objectDepth > 0.0 && objectDepth < 1.0;

    return (objectDepth > (shadowDepth + SHADOW_BIAS)) && liesInShadowMap;
}

/**
 * @brief Calculate a shadow strength for a shadow map that is smooth at
 *        the shadow's edges.
 */
float calcSmoothShadowStrength(vec3 worldCoords, uint shadowIndex)
{
    vec4 projCoords = SHADOW_MATRICES[shadowIndex] * vec4(worldCoords, 1.0);
    projCoords.xyz /= projCoords.w;

    const vec2 shadowMapUV = projCoords.xy * 0.5 + 0.5;
    const float objectDepth = projCoords.z; // Don't transform from [-1, 1] to [0, 1] because depth
                                            // is already in the range [0, 1] in Vulkan

    // Exit early if the middle fragment isn't inside of the shadow map
    if (!(shadowMapUV.x > 0.0 && shadowMapUV.x < 1.0
          && shadowMapUV.y > 0.0 && shadowMapUV.y < 1.0
          && objectDepth > 0.0 && objectDepth < 1.0))
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
            bool objectIsInShadow = objectDepth > (shadowDepth + SHADOW_BIAS);

            shadowStrength += 1.0 * float(objectIsInShadow && liesInShadowMap);
        }
    }

    return shadowStrength / 9.0;
}

/**
 * @brief Calculate the amount of light that reaches a point
 */
float calcSunShadowValueSharp(vec3 worldCoords, Light light)
{
    if (!HAS_SHADOW(light) || light.type != LIGHT_TYPE_SUN) {
        return NO_SHADOW;
    }

    return float(!isInShadow(worldCoords, light.shadowMapIndices[0]));
}

/**
 * @brief Calculate the amount of light that reaches a point
 *
 * Smoothes the shadow at the edges
 */
float calcSunShadowValueSmooth(vec3 worldCoords, Light light)
{
    if (!HAS_SHADOW(light) || light.type != LIGHT_TYPE_SUN) {
        return NO_SHADOW;
    }

    return 1.0 - calcSmoothShadowStrength(worldCoords, light.shadowMapIndices[0]);
}

#endif
