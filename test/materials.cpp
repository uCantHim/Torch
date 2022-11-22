#include <cstring>

#include <iostream>

#include <shader_tools/ShaderDocument.h>
#include <spirv/CompileSpirv.h>
#include <trc/DrawablePipelines.h>
#include <trc/PipelineDefinitions.h>
#include <trc/Torch.h>
#include <trc/TorchRenderStages.h>
#include <trc/assets/import/AssetImport.h>
#include <trc/core/Pipeline.h>
#include <trc/core/PipelineLayoutBuilder.h>
#include <trc/drawable/DefaultDrawable.h>
#include <trc/material/FragmentShader.h>
#include <trc/material/MaterialCompiler.h>
#include <trc/material/MaterialRuntime.h>
#include <trc/material/Mix.h>
#include <trc/material/VertexShader.h>

using namespace trc;

auto makeFragmentCapabiltyConfig() -> ShaderCapabilityConfig;
auto makeMaterial(ShaderModuleBuilder& builder,
                  MaterialOutputNode& materialNode,
                  PipelineVertexParams vertParams,
                  PipelineFragmentParams fragParams) -> MaterialRuntimeInfo;
void run(MaterialRuntimeInfo material);

enum InputParam
{
    eColor,
    eNormal,
    eRoughness,
};

class TangentToWorldspace : public ShaderFunction
{
public:
    TangentToWorldspace()
        : ShaderFunction("TangentspaceToWorldspace", FunctionType{ { vec3{} }, vec3{} })
    {
    }

    void build(ShaderModuleBuilder& builder, std::vector<code::Value> args) override
    {
        auto movedInterval = builder.makeSub(
            builder.makeMul(args[0], builder.makeConstant(2.0f)),
            builder.makeConstant(1.0f)
        );
        builder.makeReturn(
            builder.makeMul(
                builder.makeCapabilityAccess(FragmentCapability::kTangentToWorldSpaceMatrix),
                movedInterval
            )
        );
    }
};

int main()
{
    AssetReference<Texture> tex(std::make_unique<InMemorySource<Texture>>(
        loadTexture(TRC_TEST_ASSET_DIR"/lena.png")
    ));
    AssetReference<Texture> normalMap(std::make_unique<InMemorySource<Texture>>(
        loadTexture(TRC_TEST_ASSET_DIR"/rough_stone_wall_normal.tif")
    ));

    // Create output node
    MaterialOutputNode mat;
    auto inColor = mat.addParameter(vec4{});
    auto inNormal = mat.addParameter(vec3{});
    auto inRoughness = mat.addParameter(float{});
    auto inEmissive = mat.addParameter(bool{});
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

    auto sampledNormal = builder.makeMemberAccess(
        builder.makeTextureSample({ normalMap }, uvs),
        "rgb"
    );
    auto normal = builder.makeCall<TangentToWorldspace>({ sampledNormal });

    mat.setParameter(inColor, mix);
    mat.setParameter(inNormal, normal);
    mat.setParameter(inRoughness, builder.makeConstant(0.4f));
    mat.setParameter(inEmissive,
        builder.makeExternalCall(
            "float",
            { builder.makeNot(builder.makeConstant(false)) }  // performLighting flag
        )
    );

    // Create a pipeline
    PipelineVertexParams vert{ .animated=false };
    PipelineFragmentParams frag{
        .transparent=false,
        .colorParam=inColor,
        .normalParam=inNormal,
        .roughnessParam=inRoughness,
        .emissiveParam=inEmissive
    };
    MaterialRuntimeInfo materialRuntime = makeMaterial(builder, mat, vert, frag);

    std::cout << materialRuntime.getShaderGlslCode(vk::ShaderStageFlagBits::eFragment);
    std::cout << "\n--- vertex shader ---\n";
    std::cout << materialRuntime.getShaderGlslCode(vk::ShaderStageFlagBits::eVertex);

    run(std::move(materialRuntime));

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
        materialNode.linkOutput(fragParams.emissiveParam, outMaterial, "[3]");
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
    VertexShaderBuilder vertBuilder(material, vertParams.animated);
    auto [vert, runtimeConfig] = vertBuilder.buildVertexShader();

    // Create result value
    MaterialRuntimeInfo result{
        runtimeConfig,
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

    auto vWorldPos  = config.addResource(ShaderCapabilityConfig::ShaderInput{ vec3{}, 0 });
    auto vUv        = config.addResource(ShaderCapabilityConfig::ShaderInput{ vec2{}, 1 });
    auto vMaterial  = config.addResource(ShaderCapabilityConfig::ShaderInput{ uint{}, 2, true });
    auto vTbnMat    = config.addResource(ShaderCapabilityConfig::ShaderInput{ mat3{}, 3 });

    config.linkCapability(FragmentCapability::kVertexWorldPos, vWorldPos, vec3{});
    config.linkCapability(FragmentCapability::kVertexUV, vUv, vec2{});
    config.linkCapability(FragmentCapability::kTangentToWorldSpaceMatrix, vTbnMat, mat3{});
    config.linkCapability(
        FragmentCapability::kVertexNormal,
        code.makeArrayAccess(config.accessResource(vTbnMat), code.makeConstant(2)),
        vec3{}, { vTbnMat }
    );

    return config;
}

void run(MaterialRuntimeInfo material)
{
    auto torch = trc::initFull();
    auto& assetManager = torch->getAssetManager();

    Pipeline::ID pipeline = material.makePipeline(assetManager);

    Scene scene;
    Camera camera;
    camera.makePerspective(16.0f / 9.0f, 45.0f, 0.01f, 100.0f);
    camera.lookAt(vec3(0, 1, 4), vec3(0.0f), vec3(0, 1, 0));

    scene.getLights().makeSunLight(vec3(1.0f), vec3(1, -1, -1), 0.6f);

    // Load resources
    auto geo = assetManager.create(makeCubeGeo());

    // Create drawable
    GeometryHandle geoHandle = geo.getDeviceDataHandle();
    trc::Node node;
    const RuntimePushConstantHandler& runtime = material.getPushConstantHandler();

    scene.registerDrawFunction(
        gBufferRenderStage, GBufferPass::SubPasses::gBuffer,
        pipeline,
        [&](const DrawEnvironment& env, vk::CommandBuffer cmdBuf)
        {
            auto layout = *env.currentPipeline->getLayout();
            runtime.pushConstants(
                cmdBuf, layout,
                DrawablePushConstIndex::eModelMatrix, node.getGlobalTransform()
            );

            geoHandle.bindVertices(cmdBuf, 0);
            cmdBuf.drawIndexed(geoHandle.getIndexCount(), 1, 0, 0, 0);
        }
    );

    trc::Timer timer;
    while (torch->getWindow().isOpen())
    {
        trc::pollEvents();
        node.setRotation(glm::half_pi<float>() * timer.duration() * 0.001f, vec3(0, 1, 0));

        torch->drawFrame(camera, scene);
    }

    trc::terminate();
}
