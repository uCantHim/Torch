#version 460
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require

#include "material.glsl"
#define SHADOW_DESCRIPTOR_SET_BINDING 4
#include "shadow.glsl"

layout (input_attachment_index = 0, set = 2, binding = 0) uniform subpassInput vertexPosition;
layout (input_attachment_index = 1, set = 2, binding = 1) uniform subpassInput vertexNormal;
layout (input_attachment_index = 2, set = 2, binding = 2) uniform subpassInput vertexUv;
layout (input_attachment_index = 3, set = 2, binding = 3) uniform subpassInput materialIndex;

layout (set = 0, binding = 1) restrict readonly uniform GlobalDataBuffer
{
    vec2 mousePos;
    vec2 resolution;
} global;

layout (set = 1, binding = 0, std430) restrict readonly buffer MaterialBuffer
{
    Material materials[];
};

layout (set = 1, binding = 1) uniform sampler2D textures[];

layout (set = 2, binding = 4, r32ui) uniform uimage2D fragmentListHeadPointer;

layout (set = 2, binding = 5) restrict buffer FragmentListAllocator
{
    uint nextFragmentListIndex;
};

layout (set = 2, binding = 6) restrict buffer FragmentList
{
    /**
     * 0: A packed color
     * 1: Fragment depth value
     * 2: Next-pointer
     */
    uint fragmentList[][3];
};

layout (set = 3, binding = 0) restrict readonly buffer LightBuffer
{
    uint numLights;
    Light lights[];
};

layout (push_constant) uniform PushConstants
{
    vec3 cameraPos;
} pushConstants;

layout (location = 0) out vec4 fragColor;


/////////////////////
//      Main       //
/////////////////////

uint matIndex = floatBitsToUint(subpassLoad(materialIndex).r);

vec3 calcLighting(vec3 color);

void main()
{
    vec3 color = vec3(0.3, 1.0, 0.9);

    // Use diffuse texture if available
    uint diffTexture = materials[matIndex].diffuseTexture;
    if (diffTexture != NO_TEXTURE)
    {
        vec2 uv = subpassLoad(vertexUv).xy;
        color = texture(textures[diffTexture], uv).rgb;
    }

    fragColor = vec4(calcLighting(color), 1.0);

    // Exchange doesn't seem to have any difference in performance to imageLoad().
    // Use exchange to reset head pointer to default value.
    uint fragListIndex = imageAtomicExchange(fragmentListHeadPointer, ivec2(gl_FragCoord.xy), ~0u);
    if (fragListIndex != ~0u)
    {
        fragColor *= unpackUnorm4x8(fragmentList[fragListIndex][0]);
        return;
    }
}


vec3 calcLighting(vec3 color)
{
    vec3 ambient = vec3(0.0);
    vec3 diffuse = vec3(0.0);
    vec3 specular = vec3(0.0);

    const vec3 ambientFactor = materials[matIndex].colorAmbient.rgb;
    const vec3 diffuseFactor = materials[matIndex].colorDiffuse.rgb;
    const vec3 specularFactor = materials[matIndex].colorSpecular.rgb;

    const vec3 worldPos = subpassLoad(vertexPosition).xyz;
    const vec3 normal = normalize(subpassLoad(vertexNormal).xyz);
    const vec3 toEye = normalize(pushConstants.cameraPos - worldPos);

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
                        * pow(reflectAngle, materials[matIndex].shininess)  // Specular highlight
                        * specularFactor                                    // Material factor
                        * attenuation
                        // Specular gamma correction
                        * (materials[matIndex].shininess + 2.0) / (2.0 * 3.1415926535);
        }
    }

    return color * min((ambient + diffuse), vec3(1.0)) + specular;
}
