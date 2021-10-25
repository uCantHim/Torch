#include <vkb/util/Timer.h>
#include <trc/Torch.h>
#include <trc/drawable/DrawablePool.h>
#include <trc/asset_import/AssetUtils.h>
using namespace trc::basic_types;

void run()
{
    auto torch = trc::initFull({ .enableRayTracing=true });
    trc::Scene scene;
    trc::DrawablePool pool(*torch.instance);
    pool.attachToScene(scene);

    // Load assets
    auto& ar = *torch.assetRegistry;
    auto cubeGeo = ar.add(trc::makeCubeGeo());
    auto cubeMat = ar.add(trc::Material{ .color=vec4(1.0f) });
    auto planeGeo = ar.add(trc::makePlaneGeo(3.0f, 3.0f));
    auto planeMat = ar.add(trc::Material{ .color=vec4(1.0f), .kSpecular=vec4(0.0f) });
    auto lindaGeo = trc::loadGeometry(TRC_TEST_ASSET_DIR"/Female_Character.fbx", ar).get();
    auto lindaMat = ar.add(trc::Material{ .color=vec4(1, 0.2f, 0.4f, 1), .kSpecular=vec4(0.0f) });
    ar.updateMaterials();

    // Create lights and shadows
    trc::Light sun = scene.getLights().makeSunLight(vec3(1, 0.5f, 0), vec3(1, -2.0f, -0.2f), 0.3f);

    auto& shadow = scene.enableShadow(sun, trc::ShadowCreateInfo{ uvec2(2048) }, *torch.shadowPool);
    shadow.setProjectionMatrix(glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, -40.0f, 5.0f));
    scene.getRoot().attach(shadow);

    // Create drawables
    auto cube = pool.create({ cubeGeo, cubeMat });
    cube->copy()->translate(1, 0.5, 0).scale(0.3f).rotate(0.1f, 0.1f, 0.1f);
    cube->copy()->translate(-1, 0.5, 0).scale(0.3f).rotate(0.1f, 0.1f, 0.1f);
    cube->copy()->translate(0, 0.5, 0).scale(0.3f).rotate(0.1f, 0.1f, 0.1f);
    auto delInst = cube->copy();
    cube->copy()->destroy();
    cube->copy()->translate(1, 1, 0).scale(0.3f).rotate(0.1f, 0.1f, 0.1f);
    cube->copy()->translate(-1, 1, 0).scale(0.3f).rotate(0.1f, 0.1f, 0.1f);
    cube->copy()->translate(0, 1, 0).scale(0.3f).rotate(0.1f, 0.1f, 0.1f);
    cube->copy()->translate(1, 1.5, 0).scale(0.3f).rotate(0.1f, 0.1f, 0.1f);
    cube->copy()->translate(-1, 1.5, 0).scale(0.3f).rotate(0.1f, 0.1f, 0.1f);
    cube->copy()->translate(0, 1.5, 0).scale(0.3f).rotate(0.1f, 0.1f, 0.1f);
    delInst->destroy();

    pool.create({ planeGeo, planeMat })->translateY(-0.6f);
    pool.create({ planeGeo, planeMat })->destroy();

    auto linda = pool.create({ lindaGeo, lindaMat });
    linda->getAnimationEngine().playAnimation(0);
    linda->scale(0.7f).translate(1.5f, -1.0f, -2.0f);

    // Create camera
    trc::Camera camera;
    camera.lookAt(vec3(0, 1, 5), vec3(0, 0, 0), vec3(0, 1, 0));
    vec2 win = torch.window->getSwapchain().getSize();
    camera.makePerspective(win.x / win.y, 45.0f, 0.1f, 100.0f);

    vkb::Timer frameTimer;
    while (torch.window->getSwapchain().isOpen())
    {
        trc::pollEvents();

        const float frameTime = frameTimer.reset();
        scene.updateTransforms();
        linda->getAnimationEngine().update(frameTime);

        cube->rotateY((frameTime * 0.001f) * glm::radians(90.0f));

        torch.drawFrame(torch.makeDrawConfig(scene, camera));
    }
}

int main()
{
    run();

    trc::terminate();
    return 0;
}
