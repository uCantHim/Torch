#include <cstring>

#include <iostream>

#include <shader_tools/ShaderDocument.h>
#include <spirv/CompileSpirv.h>
#include <trc/DrawablePipelines.h>
#include <trc/PipelineDefinitions.h>
#include <trc/Torch.h>
#include <trc/TorchRenderStages.h>
#include <trc/assets/SimpleMaterial.h>
#include <trc/assets/import/AssetImport.h>
#include <trc/core/Pipeline.h>
#include <trc/core/PipelineLayoutBuilder.h>
#include <trc/drawable/DefaultDrawable.h>
#include <trc/material/CommonShaderFunctions.h>
#include <trc/material/FragmentShader.h>
#include <trc/material/MaterialRuntime.h>
#include <trc/material/MaterialStorage.h>
#include <trc/material/ShaderModuleCompiler.h>
#include <trc/material/TorchMaterialSettings.h>
#include <trc/material/VertexShader.h>

using namespace trc;

/**
 * Manually create a material by specifying fragment shader calculations
 */
auto createMaterial() -> MaterialData
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

    // Create a pipeline
    const bool transparent{ true };
    MaterialData materialData{ fragmentModule.build(transparent), transparent };

    return materialData;
}

int main()
{
    const auto materialData = createMaterial();

    // Initialize Torch
    auto torch = initFull(InstanceCreateInfo{ .enableRayTracing=false });
    auto& assetManager = torch->getAssetManager();

    Camera camera;
    camera.makePerspective(torch->getWindow().getAspectRatio(), 45.0f, 0.01f, 100.0f);
    camera.lookAt(vec3(0, 1, 4), vec3(0.0f), vec3(0, 1, 0));

    Scene scene;
    scene.getLights().makeSunLight(vec3(1.0f), vec3(1, -1, -1), 0.6f);

    // Load resources
    auto cubeGeo = assetManager.create(makeCubeGeo());
    auto cubeMat = assetManager.create(std::move(materialData));

    auto triGeo = assetManager.create(makeTriangleGeo());
    auto triMat = assetManager.create(makeMaterial(SimpleMaterialData{ .color=vec3(0, 1, 0.3f) }));

    // Create drawable
    Drawable cube(cubeGeo, cubeMat, scene);
    Drawable triangle(triGeo, triMat, scene);
    triangle.translate(-1.4f, 0.75f, -0.3f)
            .rotateY(0.2f * glm::pi<float>())
            .setScaleX(3.0f);

    Timer timer;
    while (torch->getWindow().isOpen())
    {
        pollEvents();
        cube.setRotation(glm::half_pi<float>() * timer.duration() * 0.001f, vec3(0, 1, 0));

        torch->drawFrame(camera, scene);
    }

    terminate();
    return 0;
}
