// Various lighting calculations

#ifndef TRC_LIGHTING_GLSL_INCLUDE
#define TRC_LIGHTING_GLSL_INCLUDE

#include "shadow.glsl"

struct LightValue
{
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

LightValue calcSunLight(vec3 worldPos, vec3 normal, vec3 toEye, float materialShininess)
{
    LightValue result;
    result.ambient = vec3(0.0);
    result.diffuse = vec3(0.0);
    result.specular = vec3(0.0);

    for (uint i = 0; i < numSunLights; i++)
    {
        const vec3 lightColor = lights[i].color.rgb;
        const vec3 toLight = -normalize(lights[i].direction.xyz);

        // Ambient
        result.ambient += lightColor * lights[i].ambientPercentage;

        // Diffuse
        const float shadow = calcSunShadowValueSmooth(worldPos, lights[i]);
        const float angle = max(0.0, dot(normal, toLight));

        result.diffuse += lightColor * angle * shadow;

        // Specular
        if (shadow == NO_SHADOW)
        {
            const float reflectAngle = max(0.0, dot(normalize(reflect(-toLight, normal)), toEye));
            result.specular +=
                lightColor
                * pow(reflectAngle, materialShininess)                // Specular highlight
                * ((materialShininess + 2.0) / (2.0 * 3.1415926535)); // Specular gamma correction
        }
    }

    return result;
}

LightValue calcPointLight(vec3 worldPos, vec3 normal, vec3 toEye, float materialShininess)
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

        // Attenuation
        float attenuation = 1.0f - lights[i].attenuationLinear * dist
                                 - lights[i].attenuationQuadratic * dist * dist;
        if (attenuation <= 0.0) {
            continue;
        }

        // Ambient
        result.ambient += lightColor * lights[i].ambientPercentage * attenuation;

        // Diffuse
        const float angle = max(0.0, dot(normal, toLight));
        result.diffuse += lightColor * angle * attenuation;

        // Specular
        const float reflectAngle = max(0.0, dot(normalize(reflect(-toLight, normal)), toEye));
        result.specular +=
            lightColor
            * attenuation
            * pow(reflectAngle, materialShininess)                // Specular highlight
            * ((materialShininess + 2.0) / (2.0 * 3.1415926535)); // Specular gamma correction
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

vec3 calcLighting(vec3 albedo, vec3 worldPos, vec3 normal, vec3 cameraPos, uint material)
{
    // This algorithm has undefined behaviour for normals N where |N| == 0
    // We exit early if that's the case.
    bvec3 nz = notEqual(normal, vec3(0.0));
    if (nz != bvec3(true, true, true)) {
    //if (!(nz.x || nz.y || nz.z)) {
        return albedo;
    }

    if (!materials[material].performLighting) {
        return albedo;
    }

    vec3 ambient = vec3(0.0);
    vec3 diffuse = vec3(0.0);
    vec3 specular = vec3(0.0);

    const vec3 toEye = normalize(cameraPos - worldPos);

    // Ambient
    ambient = calcAmbientLight();

    // Sun lighting
    LightValue sun = calcSunLight(worldPos, normal, toEye, materials[material].shininess);
    ambient += sun.ambient;
    diffuse += sun.diffuse;
    specular += sun.specular;

    // Point lighting
    LightValue point = calcPointLight(worldPos, normal, toEye, materials[material].shininess);
    ambient += point.ambient;
    diffuse += point.diffuse;
    specular += point.specular;

    // The material factors can be distributively multiplied at the end
    ambient *= materials[material].kAmbient.rgb;
    diffuse *= materials[material].kDiffuse.rgb;
    specular *= materials[material].kSpecular.rgb;

    return albedo * min((ambient + diffuse), vec3(1.0)) + specular;
}



#endif
