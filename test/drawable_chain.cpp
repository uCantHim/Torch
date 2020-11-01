#include <iostream>

#include <trc/Renderer.h>
#include <trc/Scene.h>
#include <trc/utils/Camera.h>
#include <trc/AssetRegistry.h>
#include <trc/AssetUtils.h>
#include <trc/experimental/DrawableChain.h>

using ar = trc::AssetRegistry::Named;
namespace trc {
    using namespace experimental;
}

struct ExampleEntity
{
    void update()
    {
    }

    std::unique_ptr<trc::DrawableChainElement> drawable;
};

int main()
{
    vkb::vulkanInit();

    auto geo = ar::addGeometry("grass", *trc::loadGeometry("assets/grass_lowpoly.fbx"));
    auto mat = ar::addMaterial("green", { .color=glm::vec4(0, 1, 0, 1), .kSpecular=glm::vec4(0.0f) });
    auto mat1 = ar::addMaterial("red", { .color=glm::vec4(1, 0, 0, 1), .kSpecular=glm::vec4(0.0f) });
    auto mat2 = ar::addMaterial("purple", { .color=glm::vec4(1, 0, 1, 1), .kSpecular=glm::vec4(0.0f) });
    auto mat3 = ar::addMaterial("blue", { .color=glm::vec4(0, 0, 1, 1), .kSpecular=glm::vec4(0.0f) });
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
    trc::Renderer renderer;
    trc::Scene scene;
    trc::Camera camera(vkb::getSwapchain().getAspectRatio(), 45.0f, 0.5f, 100.0f);
    camera.lookAt(glm::vec3(0, 2.5, 7), glm::vec3(0, 1, 0), glm::vec3(0, 1, 0));
    trc::Light sunLight = trc::makeSunLight(glm::vec3(1.0f), glm::vec3(0.0f, -1.0f, 0.0f), 0.6f);
    scene.addLight(sunLight);

    // Add things to scene
    virtualDrawable->attachToScene(scene);
    drawableChainRoot.attachToScene(scene);
    scene.getRoot().attach(drawableChainRoot);
    chainRoot.attachToScene(scene);
    scene.getRoot().attach(chainRoot);

    while (true)
    {
        scene.updateTransforms();
        renderer.drawFrame(scene, camera);
    }

    return 0;
}
