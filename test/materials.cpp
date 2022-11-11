#include <cstring>
#include <iostream>

#include <shader_tools/ShaderDocument.h>
#include <spirv/CompileSpirv.h>
#include <trc/DrawablePipelines.h>
#include <trc/PipelineDefinitions.h>
#include <trc/Torch.h>
#include <trc/assets/import/AssetImport.h>
#include <trc/core/Pipeline.h>
#include <trc/drawable/DefaultDrawable.h>
#include <trc/material/MaterialCompiler.h>
#include <trc/material/Mix.h>
#include <trc/material/VertexShader.h>

using namespace trc;

struct PipelineVertexParams
{
    bool animated;
};

struct PipelineFragmentParams
{
    bool transparent;
    MaterialOutputNode::ParameterID colorParam;
    MaterialOutputNode::ParameterID normalParam;
    MaterialOutputNode::ParameterID roughnessParam;
};

struct MaterialRuntimeInfo
{
    auto makePipeline(AssetManager& assetManager) -> Pipeline::ID;

    PipelineVertexParams vertParams;
    PipelineFragmentParams fragParams;
    ProgramDefinitionData program;

    std::vector<std::pair<TextureReference, ui32>> textureReferences;
};

auto makeCapabiltyConfig() -> ShaderCapabilityConfig;
auto makeVertexCapabilityConfig() -> ShaderCapabilityConfig;
auto makeMaterial(MaterialOutputNode& materialNode,
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
    MaterialGraph graph;

    auto uvs = graph.makeCapabilityAccess(FragmentCapability::kVertexUV, vec2{});
    auto texColor = graph.makeTextureSample({ tex }, uvs);

    auto color = graph.makeConstant(vec4(1, 0, 0.5, 1));
    auto alpha = graph.makeConstant(0.5f);
    auto mix = graph.makeFunctionCall(Mix<4, float>{}, { color, texColor, alpha });

    mat.setParameter(inColor, mix);
    mat.setParameter(inNormal, graph.makeCapabilityAccess(FragmentCapability::kVertexNormal, vec3{}));
    mat.setParameter(inRoughness, graph.makeConstant(0.4f));

    // Create a pipeline
    PipelineVertexParams vert{ .animated=false };
    PipelineFragmentParams frag{
        .transparent=false,
        .colorParam=inColor,
        .normalParam=inNormal,
        .roughnessParam=inRoughness
    };
    MaterialRuntimeInfo materialInfo = makeMaterial(mat, vert, frag);
    Pipeline::ID pipeline = materialInfo.makePipeline(assetManager);

    trc::terminate();
    return 0;
}

auto makeMaterial(MaterialOutputNode& materialNode,
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
    MaterialCompiler compiler(makeCapabiltyConfig());
    auto material = compiler.compile(materialNode);

    // Perform post-processing of the generated shader source
    std::string fragmentCode = material.shaderGlslCode;
    if (fragParams.transparent)
    {
        std::stringstream ss;
        ss << std::move(fragmentCode);
        shader_edit::ShaderDocument doc(ss);

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

    std::cout << fragmentCode;

    // Choose the appropriate vertex shader
    const auto vertFlags = vertParams.animated
        ? pipelines::AnimationTypeFlagBits::boneAnim
        : pipelines::AnimationTypeFlagBits::none;
    const auto vertexCode = internal::loadShader(pipelines::getDrawableVertex(vertFlags));

    // Create a vertex shader with a graph
    VertexShaderBuilder vertBuilder(material, true);
    auto vert = vertBuilder.buildVertexShader();

    std::cout << "\n// --- vertex shader --- //\n"
              << vert.shaderGlslCode;

    // Create result value
    MaterialRuntimeInfo result{
        .vertParams = vertParams,
        .fragParams = fragParams,
        .program = {
            .stages{
                { vk::ShaderStageFlagBits::eVertex, { vertexCode } },
                { vk::ShaderStageFlagBits::eFragment, { fragmentCode } },
            }
        },
        .textureReferences{}
    };
    for (auto& [tex, specIdx] : material.getRequiredTextures()) {
        result.textureReferences.emplace_back(tex, specIdx);
    }

    return result;
}

auto MaterialRuntimeInfo::makePipeline(AssetManager& assetManager) -> Pipeline::ID
{
    // Create the final program information
    SpecializationConstantStorage fragmentSpecs;
    for (auto& [tex, specIdx] : textureReferences)
    {
        AssetReference<Texture>& ref = tex.texture;
        if (!ref.hasResolvedID()) {
            ref.resolve(assetManager);
        }
        fragmentSpecs.set(specIdx, ref.getID().getDeviceDataHandle().getDeviceIndex());
    }

    const auto base = determineDrawablePipeline(DrawablePipelineInfo{
        .animated=vertParams.animated,
        .transparent=fragParams.transparent,
    });
    const auto layout = PipelineRegistry::getPipelineLayout(base);
    const auto rp = PipelineRegistry::getPipelineRenderPass(base);
    const auto basePipeline = PipelineRegistry::cloneGraphicsPipeline(base);

    PipelineTemplate newPipeline{ program, basePipeline.getPipelineData() };

    return PipelineRegistry::registerPipeline(newPipeline, layout, rp);
}

auto makeCapabiltyConfig() -> ShaderCapabilityConfig
{
    ShaderCapabilityConfig config;
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
    config.linkCapability(FragmentCapability::kTextureSample, textureResource);

    auto vWorldPos  = config.addResource(ShaderCapabilityConfig::ShaderInput{ vec3{} });
    auto vUv        = config.addResource(ShaderCapabilityConfig::ShaderInput{ vec2{} });
    auto vMaterial  = config.addResource(ShaderCapabilityConfig::ShaderInput{ uint{}, true });
    auto vTbnMat    = config.addResource(ShaderCapabilityConfig::ShaderInput{ mat3{} });

    config.linkCapability(FragmentCapability::kVertexWorldPos, vWorldPos);
    config.linkCapability(FragmentCapability::kVertexUV, vUv);
    config.linkCapability(FragmentCapability::kVertexNormal, vTbnMat);

    config.setCapabilityAccessor(FragmentCapability::kVertexNormal, "[2]");

    return config;
}
