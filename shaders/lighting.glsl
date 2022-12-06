// Various lighting calculations

#ifndef TRC_LIGHTING_GLSL_INCLUDE
#define TRC_LIGHTING_GLSL_INCLUDE

#include "shadow.glsl"

struct MaterialParams
{
    float kSpecular;
    float roughness;
    float metallicness;
};

struct LightValue
{
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

LightValue blinnPhong(vec3 toLight, vec3 toEye, vec3 normal, float roughness)
{
    LightValue result;

    const float angle = max(0.0, dot(normal, toLight));
    result.diffuse = vec3(angle);

    const vec3 halfway = normalize(toLight + toEye);
    const float reflectAngle = max(0.0, dot(halfway, normal));

    // Roughness can't be 0
    roughness = max(roughness, 0.001f);
    result.specular = vec3(
        pow(reflectAngle, 4.0f / roughness)                    // Specular highlight
        * (((1.0f / roughness) + 2.0) / (2.0 * 3.1415926535))  // Specular gamma correction
    );

    return result;
}

LightValue calcSunLight(vec3 worldPos, vec3 normal, vec3 toEye, float roughness)
{
    LightValue result;
    result.ambient = vec3(0.0);
    result.diffuse = vec3(0.0);
    result.specular = vec3(0.0);

    for (uint i = 0; i < numSunLights; i++)
    {
        const vec3 lightColor = lights[i].color.rgb;
        const vec3 toLight = -normalize(lights[i].direction.xyz);
        LightValue light = blinnPhong(toLight, toEye, normal, roughness);

        // Ambient
        result.ambient += lightColor * lights[i].ambientPercentage;

        // Diffuse
        const float shadow = calcSunShadowValueSmooth(worldPos, lights[i]);
        result.diffuse += lightColor * light.diffuse * shadow;

        // Specular
        if (shadow == NO_SHADOW) {
            result.specular += lightColor * light.specular;
        }
    }

    return result;
}

LightValue calcPointLight(vec3 worldPos, vec3 normal, vec3 toEye, float roughness)
{
    LightValue result;
    result.ambient = vec3(0.0);
    result.diffuse = vec3(0.0);
    result.specular = vec3(0.0);

    const uint base = numSunLights;
    for (uint i = base; i < base + numPointLights; i++)
    {
        const vec3 lightColor = lights[i].color.rgb;

        vec3 toLight = lights[i].position.xyz - worldPos;
        const float dist = length(toLight); // Also used for attenuation
        toLight /= dist;
        LightValue light = blinnPhong(toLight, toEye, normal, roughness);

        // Attenuation
        float attenuation = 1.0f - lights[i].attenuationLinear * dist
                                 - lights[i].attenuationQuadratic * dist * dist;
        if (attenuation <= 0.0) {
            continue;
        }

        // Ambient
        result.ambient  += lightColor * lights[i].ambientPercentage * attenuation;
        result.diffuse  += lightColor * light.diffuse * attenuation;
        result.specular += lightColor * attenuation * light.specular;
    }

    return result;
}

vec3 calcAmbientLight()
{
    // The ambient percentage has nothing to do with physical correctness,
    // thus it won't be affected by shadow.
    vec3 result = vec3(0.0);;

    const uint base = numSunLights + numPointLights;
    for (uint i = base; i < base + numAmbientLights; i++)
    {
        result += lights[i].color.rgb * lights[i].ambientPercentage;
    }

    return result;
}

vec3 calcLighting(vec3 albedo, vec3 worldPos, vec3 normal, vec3 cameraPos, MaterialParams mat)
{
    // This algorithm has undefined behaviour for normals N where |N| == 0
    // We exit early if that's the case.
    bvec3 nz = notEqual(normal, vec3(0.0));
    if (!(nz.x || nz.y || nz.z)) {
        return albedo;
    }

    vec3 ambient = vec3(0.0);
    vec3 diffuse = vec3(0.0);
    vec3 specular = vec3(0.0);

    const vec3 toEye = normalize(cameraPos - worldPos);

    // Ambient
    ambient = calcAmbientLight();

    // Sun lighting
    LightValue sun = calcSunLight(worldPos, normal, toEye, mat.roughness);
    ambient += sun.ambient;
    diffuse += sun.diffuse;
    specular += sun.specular;

    // Point lighting
    LightValue point = calcPointLight(worldPos, normal, toEye, mat.roughness);
    ambient += point.ambient;
    diffuse += point.diffuse;
    specular += point.specular;

    // The material factors can be distributively multiplied at the end
    specular *= mat.kSpecular;

    return albedo * min((ambient + diffuse), vec3(1.0)) + specular;
}



#endif
