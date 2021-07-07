#include <iostream>

#include <trc/Torch.h>
using namespace trc::basic_types;
#include <trc/Camera.h>
#include <trc/asset_import/AssetUtils.h>
#include <trc/asset_import/AssetImporter.h>
#include <trc/drawable/DrawableChain.h>

using ar = trc::AssetRegistry::Named;

int main()
{
    auto renderer = trc::init();

    auto grass = trc::AssetImporter::load(TRC_ASSET_DIR"/grass_lowpoly.fbx");

    auto geo = ar::addGeometry("grass", trc::Geometry(grass.meshes[0].mesh));
    auto mat = ar::addMaterial("green", { .color=vec4(0, 1, 0, 1), .kSpecular=vec4(0.0f) });
    auto mat1 = ar::addMaterial("red", { .color=vec4(1, 0, 0, 1), .kSpecular=vec4(0.0f) });
    auto mat2 = ar::addMaterial("purple", { .color=vec4(1, 0, 1, 1), .kSpecular=vec4(0.0f) });
    auto mat3 = ar::addMaterial("blue", { .color=vec4(0, 0, 1, 1), .kSpecular=vec4(0.0f) });
    trc::AssetRegistry::updateMaterials();

    // Virtual drawable interface
    std::unique_ptr<trc::DrawableInterface> virtualDrawable {
        std::make_unique<trc::VirtualDrawable<trc::Drawable>>(geo, mat3)
    };

    // In-place drawable chain creation
    trc::DrawableChainElement drawableChainRoot{
        trc::Drawable(geo, mat),
        trc::DrawableChainElement(
            [&]() {
                trc::Drawable result = trc::Drawable(geo, mat1);
                result.rotateX(-0.5f).translateX(1.5f);
                return result;
            }()
        )
    };
    drawableChainRoot.rotateX(-glm::half_pi<float>()).translateX(-1.0f);

    // Drawable chain creation via helper function
    trc::Drawable d2(geo, mat);
    d2.setScale(0.3f).translateY(2.0f);
    auto chainRoot = trc::makeDrawableChainRoot(trc::Drawable(geo, mat2), std::move(d2));
    chainRoot.translate(0.75f, -0.5f, 0.0f);

    // General setup
    trc::Scene scene;
    trc::Camera camera(vkb::getSwapchain().getAspectRatio(), 45.0f, 0.5f, 100.0f);
    camera.lookAt(vec3(0, 2.5, 7), vec3(0, 1, 0), vec3(0, 1, 0));
    scene.addLight(trc::makeSunLight(vec3(1.0f), vec3(0.0f, -1.0f, 0.0f), 0.6f));

    // Add things to scene
    virtualDrawable->attachToScene(scene);
    drawableChainRoot.attachToScene(scene);
    scene.getRoot().attach(drawableChainRoot);
    chainRoot.attachToScene(scene);
    scene.getRoot().attach(chainRoot);

    while (vkb::getSwapchain().isOpen())
    {
        vkb::pollEvents();

        scene.updateTransforms();
        renderer->drawFrame(scene, camera);
    }

    renderer.reset();

    return 0;
}
