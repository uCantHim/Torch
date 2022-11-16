#include <cstring>

#include <iostream>

#include <shader_tools/ShaderDocument.h>
#include <spirv/CompileSpirv.h>
#include <trc/DrawablePipelines.h>
#include <trc/PipelineDefinitions.h>
#include <trc/Torch.h>
#include <trc/assets/import/AssetImport.h>
#include <trc/core/Pipeline.h>
#include <trc/core/PipelineLayoutBuilder.h>
#include <trc/drawable/DefaultDrawable.h>
#include <trc/material/MaterialCompiler.h>
#include <trc/material/Mix.h>
#include <trc/material/VertexShader.h>
#include <trc/material/MaterialRuntime.h>

#include <trc/material/ShaderModuleBuilder.h>

using namespace trc;

auto makeFragmentCapabiltyConfig() -> ShaderCapabilityConfig;
auto makeDescriptorConfig() -> DescriptorConfig;
auto makeMaterial(ShaderModuleBuilder& builder,
                  MaterialOutputNode& materialNode,
                  PipelineVertexParams vertParams,
                  PipelineFragmentParams fragParams) -> MaterialRuntimeInfo;

enum InputParam
{
    eColor,
    eNormal,
    eRoughness,
};

int main()
{
    trc::init();
    Instance instance;
    AssetManager assetManager(instance, {});

    auto data = loadTexture(TRC_TEST_ASSET_DIR"/lena.png");
    AssetReference<Texture> tex(
        std::make_unique<InMemorySource<Texture>>(std::move(data))
    );

    // Create output node
    MaterialOutputNode mat;
    auto inColor = mat.addParameter(vec4{});
    auto inNormal = mat.addParameter(vec3{});
    auto inRoughness = mat.addParameter(float{});
    assert(inColor.index == InputParam::eColor);
    assert(inNormal.index == InputParam::eNormal);
    assert(inRoughness.index == InputParam::eRoughness);

    // Build a material graph
    ShaderModuleBuilder builder(makeFragmentCapabiltyConfig());

    auto uvs = builder.makeCapabilityAccess(FragmentCapability::kVertexUV);
    auto texColor = builder.makeTextureSample({ tex }, uvs);

    auto color = builder.makeConstant(vec4(1, 0, 0.5, 1));
    auto alpha = builder.makeConstant(0.5f);
    auto mix = builder.makeCall<Mix<4, float>>({ color, texColor, alpha });

    mat.setParameter(inColor, mix);
    mat.setParameter(inNormal, builder.makeCapabilityAccess(FragmentCapability::kVertexNormal));
    mat.setParameter(inRoughness, builder.makeConstant(0.4f));

    // Create a pipeline
    PipelineVertexParams vert{ .animated=false };
    PipelineFragmentParams frag{
        .transparent=false,
        .colorParam=inColor,
        .normalParam=inNormal,
        .roughnessParam=inRoughness
    };
    MaterialRuntimeInfo materialRuntime = makeMaterial(builder, mat, vert, frag);
    Pipeline::ID pipeline = materialRuntime.makePipeline(assetManager);

    std::cout << materialRuntime.getShaderGlslCode(vk::ShaderStageFlagBits::eFragment);
    std::cout << "\n--- vertex shader ---\n";
    std::cout << materialRuntime.getShaderGlslCode(vk::ShaderStageFlagBits::eVertex);

    trc::terminate();
    return 0;
}

auto makeMaterial(ShaderModuleBuilder& builder,
                  MaterialOutputNode& materialNode,
                  PipelineVertexParams vertParams,
                  PipelineFragmentParams fragParams) -> MaterialRuntimeInfo
{
    // Add attachment outputs if the material is not transparent, in which
    // case we calculate shading immediately and append the result to the
    // fragment list
    if (!fragParams.transparent)
    {
        auto outNormal = materialNode.addOutput(0, vec3{});
        auto outAlbedo = materialNode.addOutput(1, vec4{});
        auto outMaterial = materialNode.addOutput(2, vec4{});

        materialNode.linkOutput(fragParams.colorParam, outAlbedo, "");
        materialNode.linkOutput(fragParams.normalParam, outNormal, "");
        materialNode.linkOutput(fragParams.roughnessParam, outMaterial, "[1]");
    }

    // Compile the material graph
    MaterialCompiler compiler;
    auto material = compiler.compile(materialNode, builder);

    // Perform post-processing of the generated shader source
    std::string fragmentCode = material.getShaderGlslCode();
    if (fragParams.transparent)
    {
        shader_edit::ShaderDocument doc(fragmentCode);

        doc.set(material.getOutputPlaceholderVariableName(), R"(
    MaterialParams mat;
    mat.kSpecular = materials[vert.material].kSpecular;
    mat.roughness = materials[vert.material].roughness;
    mat.metallicness = materials[vert.material].metallicness;

    vec3 color = calcLighting()"
        + material.getParameterResultVariableName(fragParams.colorParam).value() + ",\n"
        + "vert.worldPos,\n"
        + material.getParameterResultVariableName(fragParams.normalParam).value() + ",\n"
        + "camera.inverseViewMatrix[3].xyz,\n"
        + "mat);\n"
        + "appendFragment(vec4(color, diffuseColor.a));"
        );

        fragmentCode = doc.compile();
    }

    // Create a vertex shader with a graph
    VertexShaderBuilder vertBuilder(material, true);
    auto vert = vertBuilder.buildVertexShader();

    // Create result value
    MaterialRuntimeInfo result{
        makeDescriptorConfig(),
        vertParams,
        fragParams,
        {
            { vk::ShaderStageFlagBits::eVertex, std::move(vert) },
            { vk::ShaderStageFlagBits::eFragment, std::move(material) },
        }
    };

    return result;
}

auto makeFragmentCapabiltyConfig() -> ShaderCapabilityConfig
{
    ShaderCapabilityConfig config;
    auto& code = config.getCodeBuilder();

    auto textureResource = config.addResource(ShaderCapabilityConfig::DescriptorBinding{
        .setName="asset_registry",
        .bindingIndex=1,
        .descriptorType="uniform sampler2D",
        .descriptorName="textures",
        .isArray=true,
        .arrayCount=0,
        .layoutQualifier=std::nullopt,
        .descriptorContent=std::nullopt,
    });
    config.addShaderExtension(textureResource, "GL_EXT_nonuniform_qualifier");
    config.linkCapability(FragmentCapability::kTextureSample, textureResource, uint{});

    auto vWorldPos  = config.addResource(ShaderCapabilityConfig::ShaderInput{ vec3{} });
    auto vUv        = config.addResource(ShaderCapabilityConfig::ShaderInput{ vec2{} });
    auto vMaterial  = config.addResource(ShaderCapabilityConfig::ShaderInput{ uint{}, true });
    auto vTbnMat    = config.addResource(ShaderCapabilityConfig::ShaderInput{ mat3{} });

    config.linkCapability(FragmentCapability::kVertexWorldPos, vWorldPos, vec3{});
    config.linkCapability(FragmentCapability::kVertexUV, vUv, vec2{});
    config.linkCapability(
        FragmentCapability::kVertexNormal,
        code.makeArrayAccess(config.accessResource(vTbnMat), code.makeConstant(2)),
        vec3{}, { vTbnMat }
    );

    return config;
}

auto makeDescriptorConfig() -> DescriptorConfig
{
    DescriptorConfig conf;
    conf.descriptorInfos.try_emplace("global_data", 0, true);
    conf.descriptorInfos.try_emplace("asset_registry", 1, true);

    return conf;
}
