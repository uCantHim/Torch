#include <trc/Torch.h>
using namespace trc::basic_types;

void run()
{
    // Create basic Torch objects
    trc::Instance instance;
    trc::Window window(instance);
    trc::RenderTarget renderTarget = trc::makeRenderTarget(window);

    trc::AssetRegistry assets(instance);
    trc::ShadowPool shadows(window, trc::ShadowPoolCreateInfo{ .maxShadowMaps=1 });

    // Create one render configuration for each viewport.
    // This also means that it would be possible to use different rendering
    // techniques for individual viewports; for example, one viewport could
    // use the standard deferred rendering algorithm while the other uses
    // ray tracing.
    trc::TorchRenderConfigCreateInfo info{
        .renderGraph=trc::makeDeferredRenderGraph(),
        .target=renderTarget,
        .assetRegistry=&assets,
        .shadowPool=&shadows
    };
    trc::TorchRenderConfig config1(window, info);
    trc::TorchRenderConfig config2(window, info);
    config1.setViewport({ 100, 200 }, { 400, 400 });
    config2.setViewport({ 400, 550 }, { 300, 150 });
    config1.setClearColor(vec4(1, 1, 0, 1));
    config2.setClearColor(vec4(0.2f, 0.5f, 1.0f, 1));

    // Recreate render target when swapchain is recreated
    vkb::on<vkb::SwapchainRecreateEvent>([&](auto) {
        renderTarget = trc::makeRenderTarget(window);
        config1.setRenderTarget(renderTarget);
        config2.setRenderTarget(renderTarget);
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
    auto planeGeo = assets.add(trc::makePlaneGeo());
    auto cubeGeo = assets.add(trc::makeCubeGeo());
    auto greenMat = assets.add(trc::Material{ .color=vec4(0, 1, 0, 1) });
    auto redMat = assets.add(trc::Material{ .color=vec4(1, 0.3f, 0, 1) });

    // Create an inclined plane and a rotating cube
    trc::Drawable plane(planeGeo, greenMat, scene);
    plane.rotateX(glm::radians(45.0f)).rotateY(glm::radians(-45.0f)).translate(0.5f, -0.5f, -1.5f);

    trc::Drawable cube(cubeGeo, redMat, scene);
    cube.scale(0.7f);

    // Create two draw configurations because we draw two viewports
    trc::DrawConfig drawConfigs[]{
        { &scene, &camera1, &config1 },  // Viewport with perspective camera
        { &scene, &camera2, &config2 },  // Viewport with orthogonal camera
    };

    trc::Timer timer;
    while (window.isOpen() && window.getKeyState(vkb::Key::escape) != vkb::InputAction::press)
    {
        trc::pollEvents();

        cube.rotateY(timer.reset() * 0.001f);

        // Specify two draws
        window.drawFrame({ 2, drawConfigs });
    }

    window.getRenderer().waitForAllFrames();
}

int main()
{
    run();
    trc::terminate();

    return 0;
}
