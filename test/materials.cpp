#include <iostream>
#include <sstream>

#include <shader_tools/ShaderDocument.h>
#include <spirv/CompileSpirv.h>
#include <trc/Torch.h>
#include <trc/assets/SimpleMaterial.h>
#include <trc/assets/import/AssetImport.h>
#include <trc/drawable/DrawableScene.h>
#include <trc/material/FragmentShader.h>
#include <trc/material/ShaderFunctions.h>
#include <trc/material/TorchMaterialSettings.h>
#include <trc/material/shader/CommonShaderFunctions.h>
#include <trc_util/Timer.h>

using namespace trc;

/**
 * Manually create a material by specifying fragment shader calculations
 */
auto createMaterial(AssetManager& assetManager) -> MaterialData
{
    auto importTex = [&](fs::path filePath) -> AssetPath
    {
        const AssetPath path(filePath.filename().replace_extension(".ta"));
        assetManager.getDataStorage().store(path, *importTexture(filePath));
        return path;
    };

    const auto lenaPath = importTex(TRC_TEST_ASSET_DIR"/lena.png");
    const auto stonePath = importTex(TRC_TEST_ASSET_DIR"/rough_stone_wall_normal.tif");
    AssetReference<Texture> tex(lenaPath);
    AssetReference<Texture> normalMap(stonePath);

    // Build a material graph
    auto buildShader = [tex, normalMap](shader::ShaderModuleBuilder& builder,
                                        FragmentModuleCreateInfo createInfo)
    {
        auto uvs = builder.makeCapabilityAccess(MaterialCapability::kVertexUV);
        auto texColor = builder.makeCall<TextureSample>({
            builder.makeSpecializationConstant(std::make_shared<RuntimeTextureIndex>(tex)),
            uvs
        });

        auto color = builder.makeCall<shader::Mix<4, float>>({
            builder.makeConstant(vec4(1, 0, 0, 1)),
            builder.makeConstant(vec4(0, 0, 1, 1)),
            builder.makeExternalCall("length", { uvs }),
        });
        auto c = builder.makeConstant(0.5f);
        auto mix = builder.makeCall<shader::Mix<4, float>>({ color, texColor, c });
        mix = builder.makeConstructor<vec4>(
            builder.makeMemberAccess(mix, "rgb"),
            builder.makeConstant(0.3f)
        );

        auto sampledNormal = builder.makeMemberAccess(
            builder.makeCall<TextureSample>({
                builder.makeSpecializationConstant(std::make_shared<RuntimeTextureIndex>(normalMap)),
                uvs
            }),
            "rgb"
        );
        auto normal = builder.makeCall<TangentToWorldspace>({ sampledNormal });

        using Param = FragmentModule::Out;
        FragmentModule fragmentModule{ createInfo };
        fragmentModule.setParameter(Param::color, mix);
        fragmentModule.setParameter(Param::normal, normal);
        fragmentModule.setParameter(Param::specularFactor, builder.makeConstant(1.0f));
        fragmentModule.setParameter(Param::metallicness, builder.makeConstant(0.0f));
        fragmentModule.setParameter(Param::roughness, builder.makeConstant(0.4f));

        return fragmentModule;
    };

    trc::shader::ShaderModuleBuilder fragmentBuilder{ makeFragmentCapabilityConfig() };
    trc::shader::ShaderModuleBuilder closestHitBuilder{ makeRayHitCapabilityConfig() };

    FragmentModuleCreateInfo fragmentConfig{
        .transparent=true,
    };

    // Build the corresponding closest hit module, just for fun
    {
        Timer timer;
        auto closestHitModule = buildShader(closestHitBuilder, fragmentConfig);
        closestHitModule.buildClosesthitShader(closestHitBuilder);
        const auto time = timer.reset();
        std::cout << "--- Also generated a closest hit shader for the material"
                     " in " << time << " ms\n";
    }

    // Create a pipeline
    auto fragmentModule = buildShader(fragmentBuilder, fragmentConfig);
    MaterialData materialData{ {fragmentModule.build(std::move(fragmentBuilder)), fragmentConfig.transparent} };

    return materialData;
}

int main()
{
    // Initialize Torch
    auto torch = initFull(
        TorchStackCreateInfo{ .assetStorageDir=TRC_TEST_ASSET_DIR },
        InstanceCreateInfo{ .enableRayTracing=false }
    );
    auto& assetManager = torch->getAssetManager();

    auto camera = std::make_shared<Camera>();
    camera->makePerspective(torch->getWindow().getAspectRatio(), 45.0f, 0.01f, 100.0f);
    camera->lookAt(vec3(0, 1, 4), vec3(0.0f), vec3(0, 1, 0));

    auto scene = std::make_shared<DrawableScene>();
    auto light = scene->getLights().makeSunLight(vec3(1.0f), vec3(1, -1, -1), 0.6f);

    // Create a fancy material
    auto materialData = createMaterial(assetManager);

    // Demonstrate serialization and deserialization
    {
        std::stringstream stream;
        AssetSerializerTraits<Material>::serialize(materialData, stream);
        stream.flush();
        materialData = AssetSerializerTraits<Material>::deserialize(stream).value();
    }

    // Load resources
    auto cubeGeo = assetManager.create(makeCubeGeo());
    auto cubeMat = assetManager.create(std::move(materialData));

    auto triGeo = assetManager.create(makeTriangleGeo());
    auto triMat = assetManager.create(makeMaterial(SimpleMaterialData{ .color=vec3(0, 1, 0.3f) }));

    // Create drawable
    auto cube = scene->makeDrawable({ cubeGeo, cubeMat });
    auto triangle = scene->makeDrawable({ triGeo, triMat });
    triangle->translate(-1.4f, 0.75f, -0.3f)
            .rotateY(0.2f * glm::pi<float>())
            .setScaleX(3.0f);

    Timer timer;
    auto vp = torch->makeFullscreenViewport(camera, scene);
    while (torch->getWindow().isOpen())
    {
        pollEvents();
        cube->setRotation(glm::half_pi<float>() * timer.duration() * 0.001f, vec3(0, 1, 0));

        torch->drawFrame(vp);
    }

    terminate();
    return 0;
}
