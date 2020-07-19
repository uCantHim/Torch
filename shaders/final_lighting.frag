#version 460
#extension GL_GOOGLE_include_directive : require

#include "light.glsl"
#include "material.glsl"

layout (location = 0) out vec4 fragColor;

layout (set = 0, binding = 1, std140) buffer readonly LightBuffer
{
    uint numLights;
    Light lights[];
};

layout (set = 1, binding = 0, std140) buffer readonly MaterialBuffer
{
    Material materials[];
};

layout (input_attachment_index = 0, set = 2, binding = 0) uniform subpassInput vertexPosition;
layout (input_attachment_index = 1, set = 2, binding = 1) uniform subpassInput vertexNormal;
layout (input_attachment_index = 2, set = 2, binding = 2) uniform subpassInput vertexUv;
layout (input_attachment_index = 3, set = 2, binding = 3) uniform subpassInput materialIndex;


/////////////////////
//      Main       //
/////////////////////

uint mat = uint(subpassLoad(materialIndex).r);

vec3 calcLighting(vec3 color);

void main()
{
    if (subpassLoad(vertexPosition).w != 1.0)
    {
        fragColor = vec4(0.0);
        return;
    }

    uint diffTexture = materials[mat].diffuseTexture;
    vec3 color = vec3(0.3, 1.0, 0.9);

    fragColor = vec4(calcLighting(color), 1.0);
}


vec3 calcLighting(vec3 color)
{
    vec3 ambient = vec3(0.0);
    vec3 diffuse = vec3(0.0);
    vec3 specular = vec3(0.0);

    const vec3 ambientFactor = materials[mat].colorAmbient.rgb;
    const vec3 diffuseFactor = materials[mat].colorDiffuse.rgb;
    const vec3 specularFactor = materials[mat].colorSpecular.rgb;

    const vec3 normal = normalize(subpassLoad(vertexNormal).xyz);
    const vec3 toEye = -normalize(subpassLoad(vertexPosition).xyz);

    for (uint i = 0; i < numLights; i++)
    {
        const vec3 lightColor = lights[i].color.rgb;

        if (lights[i].type == LIGHT_TYPE_AMBIENT)
        {
            ambient += lightColor * ambientFactor;
            continue;
        }

        vec3 toLight;

        if (lights[i].type == LIGHT_TYPE_SUN)
        {
            toLight = -normalize(lights[i].direction.xyz);
        }

        // Diffuse
        float angle = max(0.0, dot(normal, toLight));
        diffuse += lightColor * angle * diffuseFactor;

        // Specular
        float reflectAngle = max(0.0, dot(normalize(reflect(-toLight, normal)), toEye));
        specular += lightColor * pow(reflectAngle, materials[mat].shininess) * specularFactor;
    }

    return color * (ambient + diffuse) + specular;
}
