#include <trc/AssetDescriptor.h>
#include <trc/Torch.h>
#include <trc/TorchRenderStages.h>
#include <trc/util/NullDataStorage.h>
using namespace trc::basic_types;

void run()
{
    trc::init();

    // Create basic Torch objects
    trc::Instance instance;
    trc::Window window(instance);
    trc::RenderTarget renderTarget = trc::makeRenderTarget(window);

    trc::AssetManager assets(std::make_shared<trc::NullDataStorage>());

    auto& device = instance.getDevice();
    auto assetDescriptor = trc::makeAssetDescriptor(
        instance,
        assets.getDeviceRegistry(),
        trc::AssetDescriptorCreateInfo{
            // TODO: Put these settings into a global configuration object
            .maxGeometries = 5000,
            .maxTextures = 2000,
            .maxFonts = 50,
        }
    );
    auto shadowPool = std::make_shared<trc::ShadowPool>(
        device,
        renderTarget.getFrameClock(),
        trc::ShadowPoolCreateInfo{ .maxShadowMaps=1 }
    );

    // Create one render configuration for each viewport.
    // This also means that it would be possible to use different rendering
    // techniques for individual viewports; for example, one viewport could
    // use the standard deferred rendering algorithm while the other uses
    // ray tracing.
    trc::TorchRenderConfigCreateInfo info{
        .assetRegistry=assets.getDeviceRegistry(),
        .assetDescriptor=assetDescriptor,
        .shadowDescriptor=shadowPool,
    };

    auto config = trc::makeTorchRenderConfig(instance, window.getFrameCount() * 2, info);

    auto vp1 = config.makeViewports(device, renderTarget, { 100, 200 }, { 400, 400 });
    auto vp2 = config.makeViewports(device, renderTarget, { 400, 550 }, { 300, 150 });
    //vp1.setClearColor(vec4(1, 1, 0, 1));
    //vp2.setClearColor(vec4(0.2f, 0.5f, 1.0f, 1));

    // Recreate render target when swapchain is recreated
    window.addCallbackOnResize([&](trc::Swapchain& swapchain) {
        renderTarget = trc::makeRenderTarget(swapchain);
        vp1 = { window };  // Release resources first
        vp2 = { window };  // Release resources first
        vp1 = config.makeViewports(device, renderTarget, { 100, 200 }, { 400, 400 });
        vp2 = config.makeViewports(device, renderTarget, { 400, 550 }, { 300, 150 });
    });

    // Render the same scene to both viewports, but from different cameras
    trc::Scene scene;
    trc::Camera camera1;
    trc::Camera camera2;

    camera1.lookAt(vec3(-3, 1, 1), vec3(0), vec3(0, 1, 0));
    camera1.makePerspective(400.0f / 400.0f, 45.0f, 0.1f, 10.0f);
    camera2.lookAt(vec3(0, 0, 2), vec3(0), vec3(0, 1, 0));
    camera2.makeOrthogonal(-2.0f, 2.0f, -1.0f, 1.0f, -10.0f, 10.0f);

    // Create a light
    scene.getLights().makeSunLight(vec3(1.0f), vec3(1, -1, -1), 0.6f);

    // Create some geometries to render
    auto planeGeo = assets.create(trc::makePlaneGeo());
    auto cubeGeo = assets.create(trc::makeCubeGeo());
    auto greenMat = assets.create(trc::makeMaterial({ .color=vec4(0, 1, 0, 1) }));
    auto redMat = assets.create(trc::makeMaterial({ .color=vec4(1, 0.3f, 0, 1) }));

    // Create an inclined plane and a rotating cube
    trc::Drawable plane = scene.makeDrawable({ planeGeo, greenMat });
    plane->rotateX(glm::radians(45.0f))
        .rotateY(glm::radians(-45.0f))
        .translate(0.5f, -0.5f, -1.5f);

    trc::Drawable cube = scene.makeDrawable({ cubeGeo, redMat });
    cube->scale(0.7f);

    // Create a renderer that submits a frame to a render target
    trc::Renderer renderer{ device, window };

    trc::Timer timer;
    while (window.isOpen() && window.getKeyState(trc::Key::escape) != trc::InputAction::press)
    {
        trc::pollEvents();

        cube->rotateY(timer.reset() * 0.001f);

        // Draw viewports
        auto frame = std::make_unique<trc::Frame>();

        auto& viewport1 = **vp1;
        auto& viewport2 = **vp2;
        viewport1.update(device, scene, camera1);
        viewport2.update(device, scene, camera2);

        frame->addViewport(viewport1, scene);
        frame->addViewport(viewport2, scene);

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
