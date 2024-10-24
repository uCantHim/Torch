#ifndef TRC_MATUTILS_SIMPLEMATERIAL_H
#define TRC_MATUTILS_SIMPLEMATERIAL_H

struct MaterialParameters
{
    vec3 color;
    float specularFactor;
    float roughness;
    float metallicness;
    bool emissive;
};

#endif
