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
#include <trc/material/MaterialRuntime.h>
#include <trc/material/MaterialStorage.h>
#include <trc/material/Mix.h>
#include <trc/material/ShaderModuleCompiler.h>
#include <trc/material/VertexShader.h>

using namespace trc;

auto makeDescriptorConfig() -> ShaderDescriptorConfig;
auto makeFragmentCapabiltyConfig() -> ShaderCapabilityConfig;
void run(MaterialRuntimeInfo material);

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

    // Build a material graph
    auto capabilityConfig = makeFragmentCapabiltyConfig();
    FragmentModule fragmentModule(capabilityConfig);
    ShaderModuleBuilder& builder = fragmentModule.getBuilder();

    auto uvs = builder.makeCapabilityAccess(FragmentCapability::kVertexUV);
    auto texColor = builder.makeTextureSample({ tex }, uvs);

    auto color = builder.makeCall<Mix<4, float>>({
        builder.makeConstant(vec4(1, 0, 0, 1)),
        builder.makeConstant(vec4(0, 0, 1, 1)),
        builder.makeExternalCall("length", { uvs }),
    });
    auto c = builder.makeConstant(0.5f);
    auto mix = builder.makeCall<Mix<4, float>>({ color, texColor, c });
    builder.makeAssignment(builder.makeMemberAccess(mix, "a"), builder.makeConstant(0.3f));

    auto sampledNormal = builder.makeMemberAccess(
        builder.makeTextureSample({ normalMap }, uvs),
        "rgb"
    );
    auto normal = builder.makeCall<TangentToWorldspace>({ sampledNormal });

    using Param = FragmentModule::Parameter;
    fragmentModule.setParameter(Param::eColor, mix);
    fragmentModule.setParameter(Param::eNormal, normal);
    fragmentModule.setParameter(Param::eSpecularFactor, builder.makeConstant(1.0f));
    fragmentModule.setParameter(Param::eMetallicness, builder.makeConstant(0.0f));
    fragmentModule.setParameter(Param::eRoughness, builder.makeConstant(0.4f));
    fragmentModule.setParameter(Param::eEmissive,
        builder.makeExternalCall(
            "float",
            { builder.makeNot(builder.makeConstant(false)) }  // performLighting flag
        )
    );

    // Create a pipeline
    PipelineVertexParams vertParams{ .animated=false };
    PipelineFragmentParams fragParams{ .transparent=true };

    MaterialStorage storage;
    auto materialId = storage.registerMaterial({
        .fragmentModule=fragmentModule.build(fragParams.transparent),
        .descriptorConfig=makeDescriptorConfig(),
        .fragmentInfo=fragParams
    });
    MaterialRuntimeInfo& materialRuntime = storage.getMaterial(materialId, vertParams);

    std::cout << materialRuntime.getShaderGlslCode(vk::ShaderStageFlagBits::eFragment);
    std::cout << "\n--- vertex shader ---\n";
    std::cout << materialRuntime.getShaderGlslCode(vk::ShaderStageFlagBits::eVertex);

    run(std::move(materialRuntime));

    trc::terminate();
    return 0;
}

auto makeDescriptorConfig() -> ShaderDescriptorConfig
{
    return ShaderDescriptorConfig{
        .descriptorInfos{
            { "global_data",    { 0, true } },
            { "asset_registry", { 1, true } },
            { "scene_data",     { 2, true } },
            { "g_buffer",       { 3, true } },
            { "shadow",         { 4, true } },
        }
    };
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
    config.linkCapability(FragmentCapability::kTextureSample, textureResource);

    auto fragListPointerImageResource = config.addResource(ShaderCapabilityConfig::DescriptorBinding{
        .setName="g_buffer",
        .bindingIndex=4,
        .descriptorType="uniform uimage2D",
        .descriptorName="fragmentListHeadPointer",
        .isArray=false,
        .arrayCount=0,
        .layoutQualifier="r32ui",
        .descriptorContent=std::nullopt,
    });
    auto fragListAllocResource = config.addResource(ShaderCapabilityConfig::DescriptorBinding{
        .setName="g_buffer",
        .bindingIndex=5,
        .descriptorType="restrict buffer",
        .descriptorName="FragmentListAllocator",
        .isArray=false,
        .arrayCount=0,
        .layoutQualifier=std::nullopt,
        .descriptorContent=
            "uint nextFragmentListIndex;\n"
            "uint maxFragmentListIndex;"
    });
    auto fragListResource = config.addResource(ShaderCapabilityConfig::DescriptorBinding{
        .setName="g_buffer",
        .bindingIndex=6,
        .descriptorType="restrict buffer",
        .descriptorName="FragmentListBuffer",
        .isArray=false,
        .arrayCount=0,
        .layoutQualifier=std::nullopt,
        .descriptorContent="uvec4 fragmentList[];",
    });
    config.linkCapability(
        FragmentCapability::kNextFragmentListIndex,
        code.makeMemberAccess(config.accessResource(fragListAllocResource), "nextFragmentListIndex"),
        { fragListAllocResource }
    );
    config.linkCapability(
        FragmentCapability::kMaxFragmentListIndex,
        code.makeMemberAccess(config.accessResource(fragListAllocResource), "maxFragmentListIndex"),
        { fragListAllocResource }
    );
    config.linkCapability(FragmentCapability::kFragmentListHeadPointerImage,
                          fragListPointerImageResource);
    config.linkCapability(
        FragmentCapability::kFragmentListBuffer,
        code.makeMemberAccess(config.accessResource(fragListResource), "fragmentList"),
        { fragListResource }
    );

    auto shadowMatrixBufferResource = config.addResource(ShaderCapabilityConfig::DescriptorBinding{
        .setName="shadow",
        .bindingIndex=0,
        .descriptorType="restrict readonly buffer",
        .descriptorName="ShadowMatrixBuffer",
        .isArray=false,
        .arrayCount=0,
        .layoutQualifier=std::nullopt,
        .descriptorContent="mat4 shadowMatrices[];",
    });
    auto shadowMapsResource = config.addResource(ShaderCapabilityConfig::DescriptorBinding{
        .setName="shadow",
        .bindingIndex=1,
        .descriptorType="uniform sampler2D",
        .descriptorName="shadowMaps",
        .isArray=true,
        .arrayCount=0,
        .layoutQualifier=std::nullopt,
        .descriptorContent=std::nullopt,
    });
    config.linkCapability(
        FragmentCapability::kShadowMatrices,
        code.makeMemberAccess(config.accessResource(shadowMatrixBufferResource), "shadowMatrices"),
        { shadowMapsResource, shadowMatrixBufferResource }
    );

    auto lightBufferResource = config.addResource(ShaderCapabilityConfig::DescriptorBinding{
        .setName="scene_data",
        .bindingIndex=0,
        .descriptorType="restrict readonly buffer",
        .descriptorName="LightBuffer",
        .isArray=false,
        .arrayCount=0,
        .layoutQualifier=std::nullopt,
        .descriptorContent=
            "uint numSunLights;"
            "uint numPointLights;"
            "uint numAmbientLights;"
            "Light lights[];"
    });
    config.addShaderInclude(lightBufferResource, util::Pathlet("material_utils/light.glsl"));
    config.linkCapability(FragmentCapability::kLightBuffer, lightBufferResource);

    auto cameraBufferResource = config.addResource(ShaderCapabilityConfig::DescriptorBinding{
        .setName="global_data",
        .bindingIndex=0,
        .descriptorType="uniform",
        .descriptorName="camera",
        .isArray=false,
        .arrayCount=0,
        .layoutQualifier="std140",
        .descriptorContent=
            "mat4 viewMatrix;\n"
            "mat4 projMatrix;\n"
            "mat4 inverseViewMatrix;\n"
            "mat4 inverseProjMatrix;\n"
    });
    config.linkCapability(
        FragmentCapability::kCameraWorldPos,
        code.makeMemberAccess(
            code.makeArrayAccess(
                code.makeMemberAccess(config.accessResource(cameraBufferResource), "viewMatrix"),
                code.makeConstant(2)
            ),
            "xyz"
        ),
        { cameraBufferResource }
    );

    auto vWorldPos  = config.addResource(ShaderCapabilityConfig::ShaderInput{ vec3{}, 0 });
    auto vUv        = config.addResource(ShaderCapabilityConfig::ShaderInput{ vec2{}, 1 });
    auto vMaterial  = config.addResource(ShaderCapabilityConfig::ShaderInput{ uint{}, 2, true });
    auto vTbnMat    = config.addResource(ShaderCapabilityConfig::ShaderInput{ mat3{}, 3 });

    config.linkCapability(FragmentCapability::kVertexWorldPos, vWorldPos);
    config.linkCapability(FragmentCapability::kVertexUV, vUv);
    config.linkCapability(FragmentCapability::kTangentToWorldSpaceMatrix, vTbnMat);
    config.linkCapability(
        FragmentCapability::kVertexNormal,
        code.makeArrayAccess(config.accessResource(vTbnMat), code.makeConstant(2)),
        { vTbnMat }
    );

    return config;
}

void run(MaterialRuntimeInfo material)
{
    auto torch = trc::initFull();
    auto& assetManager = torch->getAssetManager();

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

    Pipeline::ID pipeline = material.makePipeline(assetManager);
    const RuntimePushConstantHandler& runtime = material.getPushConstantHandler();

    scene.registerDrawFunction(
        gBufferRenderStage, GBufferPass::SubPasses::transparency,
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
