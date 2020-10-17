// Various lighting calculations

#include "shadow.glsl"

vec3 calcLighting(vec3 albedo, vec3 worldPos, vec3 normal, vec3 cameraPos, uint material)
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

    const vec3 ambientFactor = materials[material].kAmbient.rgb;
    const vec3 diffuseFactor = materials[material].kDiffuse.rgb;
    const vec3 specularFactor = materials[material].kSpecular.rgb;

    const vec3 toEye = normalize(cameraPos - worldPos);

    for (uint i = 0; i < numLights; i++)
    {
        const vec3 lightColor = lights[i].color.rgb;

        if (lights[i].type == LIGHT_TYPE_AMBIENT)
        {
            ambient += lightColor * ambientFactor;
            continue;
        }

        vec3 toLight;
        float attenuation = 1.0f;
        if (lights[i].type == LIGHT_TYPE_SUN)
        {
            toLight = -normalize(lights[i].direction.xyz);
        }
        else if (lights[i].type == LIGHT_TYPE_POINT)
        {
            toLight = lights[i].position.xyz - worldPos;

            float dist = length(toLight);
            attenuation -= (lights[i].attenuationLinear * dist
                            + lights[i].attenuationQuadratic * dist * dist);
            if (attenuation <= 0.0) {
                continue;
            }

            toLight /= dist;
        }

        const float shadow = lightShadowValue(worldPos, lights[i], 0.002);

        // Ambient
        // The ambient percentage has nothing to do with physical correctness,
        // thus it won't be affected by shadow.
        ambient += lightColor * ambientFactor * lights[i].ambientPercentage * attenuation;

        // Diffuse
        float angle = max(0.0, dot(normal, toLight));
        diffuse += lightColor * angle * diffuseFactor * attenuation * shadow;

        // Specular
        if (shadow == NO_SHADOW)
        {
            float reflectAngle = max(0.0, dot(normalize(reflect(-toLight, normal)), toEye));
            specular += lightColor
                        * pow(reflectAngle, materials[material].shininess)  // Specular highlight
                        * specularFactor                                    // Material factor
                        * attenuation
                        // Specular gamma correction
                        * (materials[material].shininess + 2.0) / (2.0 * 3.1415926535);
        }
    }

    return albedo * min((ambient + diffuse), vec3(1.0)) + specular;
}
