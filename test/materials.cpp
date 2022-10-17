#include <cstring>
#include <iostream>

#include <spirv/CompileSpirv.h>
#include <trc/assets/import/AssetImport.h>
#include <trc/material/MaterialCompiler.h>
#include <trc/material/Mix.h>

using namespace trc;

auto makeCapabiltyConfig() -> ShaderCapabilityConfig;

int main()
{
    auto data = trc::loadTexture(TRC_TEST_ASSET_DIR"/lena.png");
    trc::AssetReference<trc::Texture> tex(
        std::make_unique<trc::InMemorySource<trc::Texture>>(std::move(data))
    );

    // Build a material graph
    MaterialGraph graph;

    auto uvs = graph.makeBuiltinConstant(Builtin::eVertexUV);
    auto texColor = graph.makeTextureSample({ tex }, uvs);

    auto color = graph.makeConstant(vec4(1, 0, 0.5, 1));
    auto alpha = graph.makeConstant(0.5f);
    auto mix = graph.makeFunction(Mix<4, float>{}, { color, texColor, alpha });

    graph.getResultNode().setColor(mix);

    // Compile the graph
    MaterialCompiler compiler(makeCapabiltyConfig());
    auto shaderSource = compiler.compile(graph).fragmentGlslCode;

    std::cout << shaderSource << "\n";

    return 0;
}

auto makeCapabiltyConfig() -> ShaderCapabilityConfig
{
    ShaderCapabilityConfig config;
    auto textureResource = config.addResource(ShaderCapabilityConfig::DescriptorBinding{
        .setName="asset_registry",
        .descriptorType="uniform sampler2D",
        .descriptorName="textures",
        .isArray=true,
        .arrayCount=0,
        .layoutQualifier=std::nullopt,
        .descriptorContent=std::nullopt,
    });
    config.linkCapability(Capability::eTextureSample, textureResource);

    auto vertexInput = config.addResource(ShaderCapabilityConfig::VertexInput{
        .contents=R"(
    vec3 worldPos;
    vec2 uv;
    flat uint material;
    mat3 tbn;
        )"
    });
    config.linkCapability(Capability::eVertexNormal, vertexInput);
    config.linkCapability(Capability::eVertexPosition, vertexInput);
    config.linkCapability(Capability::eVertexUV, vertexInput);
    config.setConstantAccessor(Builtin::eVertexNormal, ".tbn[2]");
    config.setConstantAccessor(Builtin::eVertexPosition, ".worldPos");
    config.setConstantAccessor(Builtin::eVertexUV, ".uv");

    return config;
}
