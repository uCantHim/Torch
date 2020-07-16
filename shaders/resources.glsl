// Definitions of various resource structs

#define NO_TEXTURE uint(~0)

struct Material
{
    vec4 colorAmbient;
    vec4 colorDiffuse;
    vec4 colorSpecular;

    float shininess;

    float opacity;
    float reflectivity;

    uint diffuseTexture;
    uint specularTexture;
    uint bumpTexture;
};
