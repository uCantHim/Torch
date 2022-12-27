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
#include <trc/material/FragmentShader.h>
#include <trc/material/MaterialRuntime.h>
#include <trc/material/MaterialStorage.h>
#include <trc/material/Mix.h>
#include <trc/material/ShaderModuleCompiler.h>
#include <trc/material/TorchMaterialSettings.h>
#include <trc/material/VertexShader.h>

using namespace trc;

void run(MaterialData material);

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
    const bool transparent{ true };
    MaterialData materialData{ fragmentModule.build(transparent), transparent };

    run(materialData);

    trc::terminate();
    return 0;
}

void run(MaterialData materialData)
{
    std::cout << materialData.fragmentModule.getGlslCode() << "\n\n";
    std::cout << VertexModule(false).build(materialData.fragmentModule).getGlslCode() << "\n\n";

    auto torch = trc::initFull(trc::InstanceCreateInfo{ .enableRayTracing=false });
    auto& assetManager = torch->getAssetManager();

    Scene scene;
    Camera camera;
    camera.makePerspective(16.0f / 9.0f, 45.0f, 0.01f, 100.0f);
    camera.lookAt(vec3(0, 1, 4), vec3(0.0f), vec3(0, 1, 0));

    scene.getLights().makeSunLight(vec3(1.0f), vec3(1, -1, -1), 0.6f);

    // Load resources
    auto geo = assetManager.create(makeCubeGeo());
    auto mat = assetManager.create(std::move(materialData));

    auto tri = assetManager.create(makeTriangleGeo());
    auto simpleMat = assetManager.create(makeMaterial(SimpleMaterialData{ .color=vec3(0, 1, 0.3f) }));

    // Create drawable
    trc::Drawable drawable(geo, mat, scene);
    trc::Drawable triangle(tri, simpleMat, scene);
    triangle.translate(-1.4f, 0.75f, -0.3f)
            .rotateY(0.2f * glm::pi<float>())
            .setScaleX(3.0f);

    trc::Timer timer;
    while (torch->getWindow().isOpen())
    {
        trc::pollEvents();
        drawable.setRotation(glm::half_pi<float>() * timer.duration() * 0.001f, vec3(0, 1, 0));

        torch->drawFrame(camera, scene);
    }

    trc::terminate();
}
