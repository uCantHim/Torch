#version 460
#extension GL_GOOGLE_include_directive : require

#include "../light.glsl"
#include "../material.glsl"

layout (set = 0, binding = 1, std140) buffer readonly LightBuffer
{
    uint numLights;
    Light lights[];
};

layout (set = 1, binding = 0, std140) buffer readonly MaterialBuffer
{
    Material materials[];
};

layout (location = 0) in Vertex
{
    vec3 worldPos;
    vec3 normal;
    vec2 uv;
    uint material;
} vert;

layout (location = 0) out vec4 fragColor;


/////////////////////
//      Main       //
/////////////////////

vec3 calcLighting(vec3 color);

void main()
{
    vec3 color = vec3(0.0);

    uint diffTexture = materials[vert.material].diffuseTexture;
    if (diffTexture == NO_TEXTURE)
    {
        color = materials[vert.material].colorDiffuse.rgb;
    }
    else
    {
        color = vec3(0, 1, 0);
    }

    color = calcLighting(color);

    fragColor = vec4(color, 1.0);
}


vec3 calcLighting(vec3 color)
{
    vec3 ambient = vec3(0.0);
    vec3 diffuse = vec3(0.0);
    vec3 specular = vec3(0.0);

    const uint mat = vert.material;
    const vec3 ambientFactor = materials[mat].colorAmbient.rgb;
    const vec3 diffuseFactor = materials[mat].colorDiffuse.rgb;
    const vec3 specularFactor = materials[mat].colorSpecular.rgb;

    const vec3 normal = normalize(vert.normal);
    const vec3 toEye = -normalize(vert.worldPos);

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
