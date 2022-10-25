#version 460
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require

#include "material.glsl"

layout (early_fragment_tests) in;

// Buffers
layout (set = 1, binding = 0, std430) restrict readonly buffer MaterialBuffer
{
    Material materials[];
};

layout (set = 1, binding = 1) uniform sampler2D textures[];

// Input
layout (location = 0) in VertexData
{
    vec3 worldPos;
    vec2 uv;
    flat uint material;
    mat3 tbn;
} vert;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec4 outAlbedo;
layout (location = 2) out vec4 outMaterial;


/////////////////////
//      Main       //
/////////////////////

vec3 calcVertexColor();
vec3 calcVertexNormal();

void main()
{
    outNormal = calcVertexNormal();
    outAlbedo = vec4(calcVertexColor(), 0.0);

    outMaterial[0] = materials[vert.material].kSpecular;    // specular coefficient
    outMaterial[1] = materials[vert.material].roughness;    // roughness
    outMaterial[2] = materials[vert.material].metallicness;    // metallicness
    outMaterial[3] = float(materials[vert.material].performLighting);
}


vec3 calcVertexColor()
{
    vec3 color = materials[vert.material].color.rgb;

    // Use diffuse texture if available
    uint diffTexture = materials[vert.material].diffuseTexture;
    if (diffTexture != NO_TEXTURE) {
        color = texture(textures[diffTexture], vert.uv).rgb;
    }

    return color;
}


vec3 calcVertexNormal()
{
    uint bumpTex = materials[vert.material].bumpTexture;
    if (bumpTex == NO_TEXTURE)
    {
        return normalize(vert.tbn[2]);
    }
    else
    {
        vec3 textureNormal = texture(textures[bumpTex], vert.uv).rgb * 2.0 - 1.0;
        return normalize(vert.tbn * textureNormal);
    }
}
