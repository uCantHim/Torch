// Light related stuff

#define LIGHT_TYPE_SUN 0
#define LIGHT_TYPE_POINT 1
#define LIGHT_TYPE_AMBIENT 2

struct Light
{
    vec4 color;
    vec4 position;
    vec4 direction;

    float ambientPercentage;

    float attenuationLinear;
    float attenuationQuadratic;

    uint type;

    bool hasShadow;
    uint firstShadowIndex;
};
