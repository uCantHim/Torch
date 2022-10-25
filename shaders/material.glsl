// Material related stuff

#ifndef TRC_MATERIAL_GLSL_INCLUDE
#define TRC_MATERIAL_GLSL_INCLUDE

#define NO_TEXTURE (uint(0) - 1)

struct Material
{
    vec4 color;

    float kSpecular;
    float roughness;
    float metallicness;

    float reflectivity;

    uint diffuseTexture;
    uint specularTexture;
    uint bumpTexture;

    bool performLighting;
};

#endif
