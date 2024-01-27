#include <iostream>

#include <trc/DescriptorSetUtils.h>
#include <trc/PipelineDefinitions.h>
#include <trc/TopLevelAccelerationStructureBuilder.h>
#include <trc/Torch.h>
#include <trc/TorchRenderStages.h>
#include <trc/base/Barriers.h>
#include <trc/base/ImageUtils.h>
#include <trc/base/event/Event.h>
#include <trc/drawable/DrawableScene.h>
#include <trc/ray_tracing/RaygenDescriptor.h>
#include <trc_util/Timer.h>
using namespace trc::basic_types;

using trc::rt::BLAS;
using trc::rt::TLAS;

void run()
{
    auto torch = trc::initFull({}, trc::InstanceCreateInfo{ .enableRayTracing = true });
    auto& window = torch->getWindow();
    auto& assets = torch->getAssetManager();

    auto scene = std::make_unique<trc::DrawableScene>();

    // Camera
    trc::Camera camera;
    camera.lookAt({ 0, 2, 4 }, { 0, 0, 0 }, { 0, 1, 0 });
    auto size = window.getImageExtent();
    camera.makePerspective(float(size.width) / float(size.height), 45.0f, 0.1f, 100.0f);


    // --- Create a scene --- //

    auto sphere = scene->makeDrawable(trc::DrawableCreateInfo{
        assets.create(trc::makeSphereGeo()),
        assets.create(trc::makeMaterial({
            .color=vec4(0.8f, 0.3f, 0.6f, 1),
            .specularCoefficient=0.3f,
            .roughness=0.0f,
        })),
        true, true
    });
    sphere->translate(-1.5f, 0.5f, 1.0f).setScale(0.2f);
    trc::Node sphereNode;
    sphereNode.attach(*sphere);
    scene->getRoot().attach(sphereNode);

    auto plane = scene->makeDrawable({
        assets.create(trc::makePlaneGeo()),
        assets.create(trc::makeMaterial({ .roughness=0.1f })),
        true, true
    });
    plane->rotate(glm::radians(90.0f), glm::radians(-15.0f), 0.0f)
         .translate(0.5f, 0.5f, -1.0f)
         .setScale(3.0f, 1.0f, 1.7f);

    trc::GeometryID treeGeo = assets.create(trc::loadGeometry(TRC_TEST_ASSET_DIR"/tree_lowpoly.fbx"));
    trc::MaterialID treeMat = assets.create(trc::makeMaterial({ .color=vec4(0, 1, 0, 1) }));
    auto tree = scene->makeDrawable({ treeGeo, treeMat, true, true });
    tree->rotateX(-glm::half_pi<float>()).setScale(0.1f);

    constexpr float kFloorReflectivity{ 0.2f };
    auto floor = scene->makeDrawable({
        assets.create(trc::makePlaneGeo(50.0f, 50.0f, 60, 60)),
        assets.create(trc::makeMaterial({
            .specularCoefficient=0.2f,
            .roughness=1.0f - kFloorReflectivity,
            .albedoTexture=assets.create(
                trc::loadTexture(TRC_TEST_ASSET_DIR"/rough_stone_wall.tif")
            ),
            .normalTexture=assets.create(
                trc::loadTexture(TRC_TEST_ASSET_DIR"/rough_stone_wall_normal.tif")
            ),
        })),
        true, true
    });

    auto& lights = scene->getModule<trc::LightSceneModule>();
    auto sun = lights.makeSunLight(vec3(1.0f), vec3(1, -1, -1), 0.5f);
    auto& shadow = lights.enableShadow(
        sun,
        { .shadowMapResolution=uvec2(2048, 2048) },
        torch->getShadowPool()
    );
    shadow.setProjectionMatrix(glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, -20.0f, 10.0f));


    // --- Set some keybindings --- //

    trc::on<trc::KeyPressEvent>([&](auto& e) {
        static bool count{ false };
        if (e.key == trc::Key::r)
        {
            //assets.getModule<trc::Material>().modify(
            //    floor.getMaterial().getDeviceID(),
            //    [](auto& mat) {
            //        mat.roughness = 1.0f - (kFloorReflectivity * float(count));
            //    }
            //);
            count = !count;
        }
    });


    // --- Run the Application --- //

    trc::Timer timer;
    trc::Timer frameTimer;
    int frames{ 0 };
    while (window.isOpen())
    {
        trc::pollEvents();

        const float time = timer.reset();
        sphereNode.rotateY(time / 1000.0f * 0.5f);
        scene->update(time);

        torch->drawFrame(camera, *scene);

        frames++;
        if (frameTimer.duration() >= 1000.0f)
        {
            std::cout << frames << " FPS\n";
            frames = 0;
            frameTimer.reset();
        }
    }
}

int main()
{
    run();

    trc::terminate();

    return 0;
}
