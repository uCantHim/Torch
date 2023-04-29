#include <trc/Torch.h>
#include <trc/TorchImplementation.h>
#include <trc/util/FilesystemDataStorage.h>
using namespace trc::basic_types;

class NullDataStorage : public trc::DataStorage
{
public:
    auto read(const path& path) -> std::shared_ptr<std::istream> override { return nullptr; }
    auto write(const path& path) -> std::shared_ptr<std::ostream> override { return nullptr; }
    bool remove(const path& path) override { return false; }
};

void run()
{
    trc::init();

    // Create basic Torch objects
    trc::Instance instance;
    trc::Window window(instance);
    trc::RenderTarget renderTarget = trc::makeRenderTarget(window);

    trc::AssetManager assets(std::make_shared<NullDataStorage>());
    trc::ShadowPool shadows(window, trc::ShadowPoolCreateInfo{ .maxShadowMaps=1 });

    auto assetDescriptor = std::make_shared<trc::AssetDescriptor>(
        trc::impl::makeDefaultAssetModules(
            instance,
            assets.getDeviceRegistry(),
            trc::AssetDescriptorCreateInfo{
                // TODO: Put these settings into a global configuration object
                .maxGeometries = 5000,
                .maxTextures = 2000,
                .maxFonts = 50,
            }
        )
    );

    // Create one render configuration for each viewport.
    // This also means that it would be possible to use different rendering
    // techniques for individual viewports; for example, one viewport could
    // use the standard deferred rendering algorithm while the other uses
    // ray tracing.
    trc::TorchRenderConfigCreateInfo info{
        .renderGraph=trc::makeDeferredRenderGraph(),
        .target=renderTarget,
        .assetRegistry=&assets.getDeviceRegistry(),
        .assetDescriptor=assetDescriptor,
        .shadowPool=&shadows
    };
    trc::TorchRenderConfig config1(window, info);
    trc::TorchRenderConfig config2(window, info);
    config1.setViewport({ 100, 200 }, { 400, 400 });
    config2.setViewport({ 400, 550 }, { 300, 150 });
    config1.setClearColor(vec4(1, 1, 0, 1));
    config2.setClearColor(vec4(0.2f, 0.5f, 1.0f, 1));

    // Recreate render target when swapchain is recreated
    trc::on<trc::SwapchainRecreateEvent>([&](auto) {
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
    auto planeGeo = assets.create<trc::Geometry>(trc::makePlaneGeo());
    auto cubeGeo = assets.create<trc::Geometry>(trc::makeCubeGeo());
    auto greenMat = assets.create<trc::Material>(trc::makeMaterial({ .color=vec4(0, 1, 0, 1) }));
    auto redMat = assets.create<trc::Material>(trc::makeMaterial({ .color=vec4(1, 0.3f, 0, 1) }));

    // Create an inclined plane and a rotating cube
    auto plane = scene.makeDrawable({ planeGeo, greenMat });
    plane->rotateX(glm::radians(45.0f)).rotateY(glm::radians(-45.0f)).translate(0.5f, -0.5f, -1.5f);

    auto cube = scene.makeDrawable({ cubeGeo, redMat });
    cube->scale(0.7f);

    // Create two draw configurations because we draw two viewports
    trc::DrawConfig drawConfigs[]{
        { scene, config1 },
        { scene, config2 },
    };

    trc::Timer timer;
    while (window.isOpen() && window.getKeyState(trc::Key::escape) != trc::InputAction::press)
    {
        trc::pollEvents();

        cube->rotateY(timer.reset() * 0.001f);

        config1.perFrameUpdate(camera1, scene);  // Viewport with perspective camera
        config2.perFrameUpdate(camera2, scene);  // Viewport with orthogonal camera

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
