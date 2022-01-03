#version 460
#extension GL_EXT_nonuniform_qualifier : require

//layout (early_fragment_tests) in;

layout (set = 1, binding = 1) uniform sampler2D textures[];

///////////////////
//      Main     //
///////////////////

layout (location = 0) in Vertex
{
    vec2 uv;
    flat uint textureIndex;
} vert;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec4 outAlbedo;
layout (location = 2) out uint outMaterial;

void main()
{
    vec4 diffuseColor = texture(textures[vert.textureIndex], vert.uv);
    if (diffuseColor.a < 0.001) {
        discard;
    }

    outNormal = vec3(0.0);
    outAlbedo = diffuseColor;
    outMaterial = 0;
}
