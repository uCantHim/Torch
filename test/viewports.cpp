#include <trc/AssetDescriptor.h>
#include <trc/Torch.h>
#include <trc/TorchRenderStages.h>
#include <trc/util/NullDataStorage.h>
#include <trc_util/Timer.h>
using namespace trc::basic_types;

void run()
{
    trc::init();

    // Create basic Torch objects
    trc::Instance instance;
    trc::Window window(instance);

    trc::AssetManager assets(std::make_shared<trc::NullDataStorage>());

    // Create a render pipeline.
    trc::TorchPipelineCreateInfo info{
        .maxViewports=2,
        .assetRegistry=assets.getDeviceRegistry(),
        .assetDescriptorCreateInfo{
            .maxGeometries = 5000,
            .maxTextures = 2000,
            .maxFonts = 50,
        },
        .maxShadowMaps=1,
    };
    auto pipeline = trc::makeTorchRenderPipeline(instance, window, info);

    window.addCallbackOnResize([&pipeline](auto& swapchain) {
        pipeline->changeRenderTarget(trc::makeRenderTarget(swapchain));
    });

    // Create a scene with two objects.
    auto scene = std::make_shared<trc::Scene>();
    auto camera1 = std::make_shared<trc::Camera>();
    auto camera2 = std::make_shared<trc::Camera>();

    camera1->lookAt(vec3(-3, 1, 1), vec3(0), vec3(0, 1, 0));
    camera1->makePerspective(400.0f / 400.0f, 45.0f, 0.1f, 10.0f);
    camera2->lookAt(vec3(0, 0, 2), vec3(0), vec3(0, 1, 0));
    camera2->makeOrthogonal(-2.0f, 2.0f, -1.0f, 1.0f, -10.0f, 10.0f);

    // Create a light
    auto light = scene->getLights().makeSunLight(vec3(1.0f), vec3(1, -1, -1), 0.6f);

    // Create some geometries to render
    auto planeGeo = assets.create(trc::makePlaneGeo());
    auto cubeGeo = assets.create(trc::makeCubeGeo());
    auto greenMat = assets.create(trc::makeMaterial({ .color=vec4(0, 1, 0, 1) }));
    auto redMat = assets.create(trc::makeMaterial({ .color=vec4(1, 0.3f, 0, 1) }));

    // Create an inclined plane and a rotating cube
    trc::Drawable plane = scene->makeDrawable({ planeGeo, greenMat });
    plane->rotateX(glm::radians(45.0f))
        .rotateY(glm::radians(-45.0f))
        .translate(0.5f, -0.5f, -1.5f);

    trc::Drawable cube = scene->makeDrawable({ cubeGeo, redMat });
    cube->scale(0.7f);

    // Create two viewports. Both draw the same scene, but from different cameras.
    auto vp1 = pipeline->makeViewport({ { 100, 200 }, { 400, 400 } }, camera1, scene, vec4(1, 1, 0, 1));
    auto vp2 = pipeline->makeViewport({ { 400, 550 }, { 300, 150 } }, camera2, scene, vec4(0.2f, 0.5f, 1.0f, 1));

    // Create a renderer that draws frames to a window.
    trc::Renderer renderer{ instance.getDevice(), window };

    trc::Timer timer;
    while (window.isOpen() && window.getKeyState(trc::Key::escape) != trc::InputAction::press)
    {
        trc::pollEvents();

        cube->rotateY(timer.reset() * 0.001f);

        // Draw viewports
        auto frame = pipeline->draw({ vp2, vp1 });

        // Dispatch the frame for rendering
        renderer.renderFrameAndPresent(std::move(frame), window);
    }

    renderer.waitForAllFrames();
}

int main()
{
    run();
    trc::terminate();

    return 0;
}
