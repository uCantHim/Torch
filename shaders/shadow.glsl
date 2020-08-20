// Functions that calculate shadows

#extension GL_GOOGLE_include_directive : require

#ifndef SHADOW_DESCRIPTOR_SET_BINDING
#define SHADOW_DESCRIPTOR_SET_BINDING 4
#endif

#define NO_SHADOW 1.0

#include "light.glsl"

layout (set = SHADOW_DESCRIPTOR_SET_BINDING, binding = 0) restrict readonly buffer ShadowMatrixBuffer
{
    mat4 shadowMatrices[];
};

layout (set = SHADOW_DESCRIPTOR_SET_BINDING, binding = 1) uniform sampler2D shadowMaps[];

/**
 * @brief Tests if a point is in a shadow of a specific light
 *
 * @param float bias Value added to the depth read from the depth texture.
 *                   A good default depth bias seems to be 0.002.
 */
bool isInShadow(vec3 worldCoords, uint shadowIndex, float bias)
{
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
float lightShadowValue(vec3 worldCoords, Light light, float bias)
{
    if (!light.hasShadow) {
        return NO_SHADOW;
    }

    switch (light.type)
    {
    case LIGHT_TYPE_SUN:
        if (isInShadow(worldCoords, light.firstShadowIndex, bias)) {
            return 0.2;
        }

        return NO_SHADOW;
    }

    return NO_SHADOW;
}
