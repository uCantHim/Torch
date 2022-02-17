#include <chrono>
using namespace std::chrono;
#include <iostream>
#include <fstream>
#include <thread>

#include <glm/gtc/random.hpp>

#include <vkb/ImageUtils.h>

#include <trc/Torch.h>
#include <trc/particle/Particle.h>
#include <trc/text/Text.h>
using namespace trc::basic_types;

void run()
{
    trc::Camera camera(1.0f, 45.0f, 0.1f, 100.0f);
    camera.lookAt({ 0.0f, 2.0f, 5.0f }, vec3(0, 0.5f, -1.0f ), { 0, 1, 0 });
    vkb::on<vkb::SwapchainResizeEvent>([&](const auto& e) {
        const auto extent = e.swapchain->getImageExtent();
        camera.setAspect(float(extent.width) / float(extent.height));
    });

    vkb::Keyboard::init();
    vkb::Mouse::init();

    auto torch = trc::initFull();
    auto& ar = torch->getRenderConfig().getAssets();
    auto& instance = torch->getInstance();
    const auto& device = instance.getDevice();

    // ------------------
    // Random test things

    auto grassImport = trc::loadGeometry(TRC_TEST_ASSET_DIR"/grass_lowpoly.fbx");
    auto treeImport = trc::loadGeometry(TRC_TEST_ASSET_DIR"/tree_lowpoly.fbx");
    auto mapImport = trc::loadGeometry(TRC_TEST_ASSET_DIR"/map.fbx");

    auto grassGeoIndex = ar.add(grassImport.meshes[0].geometry);
    auto treeGeoIndex = ar.add(treeImport.meshes[0].geometry);
    auto mapMatIndex = ar.add(mapImport.meshes[0].materials[0]);

    auto skeletonGeoIndex = trc::loadGeometry(TRC_TEST_ASSET_DIR"/skeleton.fbx", ar).get();
    auto hoodedBoiGeoIndex = trc::loadGeometry(TRC_TEST_ASSET_DIR"/hooded_boi.fbx", ar).get();
    auto lindaMesh = trc::loadGeometry(TRC_TEST_ASSET_DIR"/Female_Character.fbx").meshes[0];
    auto lindaGeoIndex = ar.add(lindaMesh.geometry, lindaMesh.rig);

    auto lindaDiffTexIdx = ar.add(
        vkb::loadImage2D(device, TRC_TEST_ASSET_DIR"/Female_Character.png")
    );

    auto grassImgIdx = ar.add(
        vkb::loadImage2D(device, TRC_TEST_ASSET_DIR"/grass_billboard_001.png")
    );
    auto stoneTexIdx = ar.add(
        vkb::makeSinglePixelImage(device, vec4(1.0f, 0.0f, 0.0f, 1.0f)) //"/rough_stone_wall.tif")
    );
    auto stoneNormalTexIdx = ar.add(
        vkb::loadImage2D(device, TRC_TEST_ASSET_DIR"/rough_stone_wall_normal.tif")
    );

    auto matIdx = ar.add({
        .kAmbient = vec4(1.0f),
        .kDiffuse = vec4(1.0f),
        .kSpecular = vec4(1.0f),
        .shininess = 2.0f,
        .diffuseTexture = grassImgIdx,
        .bumpTexture = stoneNormalTexIdx,
    });

    auto& mapMat = ar.get(mapMatIndex);
    mapMat.kAmbient = vec4(1.0f);
    mapMat.kDiffuse = vec4(1.0f);
    mapMat.kSpecular = vec4(1.0f);
    mapMat.diffuseTexture = stoneTexIdx;
    mapMat.bumpTexture = stoneNormalTexIdx;

    trc::Material treeMat{
        .color=vec4(0, 1, 0, 1),
    };
    auto treeMatIdx = ar.add(treeMat);

    // ------------------

    trc::Scene scene;

    trc::Drawable grass({ grassGeoIndex, matIdx }, scene);
    grass.setScale(0.1f).rotateX(glm::radians(-90.0f)).translateX(0.5f);

    // Animated skeleton
    trc::Node skeletonNode;
    skeletonNode.scale(0.02f).translateZ(1.2f)
                .translate(1.0f, 0.0f, 0.0f)
                .rotateY(-glm::half_pi<float>());
    scene.getRoot().attach(skeletonNode);
    std::vector<u_ptr<trc::Drawable>> skeletons;
    trc::DrawableCreateInfo skelCreateInfo{ skeletonGeoIndex, matIdx };

    for (int i = 0; i < 50; i++)
    {
        auto& inst = *skeletons.emplace_back(new trc::Drawable(skelCreateInfo, scene));
        const float angle = glm::two_pi<float>() / 50 * i;
        inst.scale(0.02f).translateZ(1.2f)
            .translate(glm::cos(angle), 0.0f, glm::sin(angle))
            .rotateY(-glm::half_pi<float>() - angle);
        scene.getRoot().attach(inst);
        inst.getAnimationEngine().playAnimation(0);
    }

    // Hooded boi
    trc::Drawable hoodedBoi({ hoodedBoiGeoIndex, {} }, scene);
    hoodedBoi.setScale(0.2f).translate(1.0f, 0.6f, -7.0f);
    hoodedBoi.getAnimationEngine().playAnimation(0);

    // Linda
    auto lindaMatIdx = ar.add({
        .kSpecular = vec4(0.0f),
        .diffuseTexture = lindaDiffTexIdx
    });

    trc::Drawable linda({ lindaGeoIndex, lindaMatIdx }, scene);
    linda.setScale(0.3f).translateX(-1.0f);
    linda.getAnimationEngine().playAnimation(0);

    // Images
    auto planeGeo = ar.add(trc::makePlaneGeo());
    auto transparentImg = ar.add(trc::Material{
        .diffuseTexture=ar.add(trc::loadImage2D(device, TRC_TEST_ASSET_DIR"/standard_model.png"))
    });
    auto opaqueImg = ar.add(trc::Material{
        .diffuseTexture=ar.add(trc::loadImage2D(device, TRC_TEST_ASSET_DIR"/lena.png"))
    });
    trc::Drawable img({ planeGeo, transparentImg, true }, scene);
    img.translate(-5, 1, -3).rotate(glm::radians(90.0f), glm::radians(30.0f), 0.0f).scale(2);
    trc::Drawable img2({ planeGeo, opaqueImg, false }, scene);
    img2.translate(-5.001f, 1, -3.001f).rotate(glm::radians(90.0f), glm::radians(30.0f), 0.0f).scale(2);

    // Generated plane geo
    auto myPlaneGeoIndex = ar.add(trc::makePlaneGeo(20.0f, 20.0f, 20, 20));
    trc::Drawable plane({ myPlaneGeoIndex, mapMatIndex }, scene);

    trc::Light sunLight = scene.getLights().makeSunLight(vec3(1.0f), vec3(1.0f, -1.0f, -1.5f));
    [[maybe_unused]]
    trc::Light ambientLight = scene.getLights().makeAmbientLight(vec3(0.15f));
    [[maybe_unused]]
    trc::Light pointLight = scene.getLights().makePointLight(vec3(1, 1, 0), vec3(2, 0.5f, 0.5f), 0.4f);

    // Sun light
    mat4 proj = glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, -50.0f, 5.0f);
    auto& shadowNode = scene.enableShadow(
        sunLight,
        { .shadowMapResolution=uvec2(2048, 2048) },
        torch->getShadowPool()
    );
    shadowNode.setProjectionMatrix(proj);
    scene.getRoot().attach(shadowNode);

    // Instanced trees
    constexpr trc::ui32 NUM_TREES = 800;
    std::vector<u_ptr<trc::Drawable>> trees;
    for (ui32 i = 0; i < NUM_TREES; i++)
    {
        auto& tree = *trees.emplace_back(new trc::Drawable({ treeGeoIndex, treeMatIdx }, scene));
        tree.setScale(0.1f).rotateX(glm::radians(-90.0f))
            .setTranslationX(-3.0f + static_cast<float>(i % 14) * 0.5f)
            .setTranslationZ(-1.0f - (static_cast<float>(i) / 14.0f) * 0.4f);
    }

    // Particles
    trc::ParticleCollection particleCollection(instance, 10000);
    particleCollection.attachToScene(scene);
    for (int i = 0; i < 1000; i++)
    {
        trc::Particle particle;
        particle.phys.position = glm::linearRand(vec3(-2, 0, -2), vec3(2, 2, 2));
        particle.phys.linearVelocity = vec3(0, 0.05f, 0);
        particle.phys.linearAcceleration = vec3(0, 0.01f, 0);
        particle.phys.angularVelocity = glm::radians(30.0f);
        particle.phys.scaling = vec3(0.15f);
        particle.phys.lifeTime = glm::linearRand(1000.0f, 6000.0f);
        particle.material.texture = grassImgIdx;
        particleCollection.addParticle(particle);
    }

    auto particleImgIdx = ar.add(vkb::loadImage2D(device, TRC_TEST_ASSET_DIR"/yellowlight.png"));
    trc::ParticleSpawn spawn(particleCollection);
    for (int i = 0; i < 50; i++)
    {
        trc::Particle p;
        p.phys.linearVelocity = glm::linearRand(vec3(0.2, 0.2, 0.2), vec3(1.5f, 1.5f, 1.0f));
        p.phys.linearAcceleration = vec3(0, -2.0f, 0);
        p.phys.scaling = vec3(0.2f);
        p.phys.lifeTime = 3000.0f;
        p.material.texture = particleImgIdx;
        p.material.blending = trc::ParticleMaterial::BlendingType::eAlphaBlend;
        spawn.addParticle(p);
    }
    spawn.spawnParticles();


    bool running{ true };
    vkb::EventHandler<vkb::SwapchainCloseEvent>::addListener(
        [&running](const vkb::SwapchainCloseEvent&) { running = false; }
    );

    std::thread particleUpdateThread([&]() {
        trc::Timer timer;
        while (running) {
            particleCollection.update(timer.reset());
        }
    });
    std::thread particleSpawnThread([&]() {
        while (running)
        {
            spawn.spawnParticles();
            std::this_thread::sleep_for(1s);
        }
    });


    // Generated cube geo
    auto cubeGeoIdx = ar.add({ trc::makeCubeGeo() });
    auto cubeMatIdx = ar.add({ .color={ 0.3, 0.3, 1, 0.5} });
    trc::Drawable cube({ cubeGeoIdx, cubeMatIdx, true }, scene);
    cube.translate(1.5f, 0.7f, 1.5f).setScale(0.3f);

    std::thread cubeRotateThread([&cube, &running]() {
        while (running)
        {
            std::this_thread::sleep_for(10ms);
            cube.rotateY(glm::radians(0.5f));
        }
    });

    // Thing at cursor
    auto cursorCubeMat = ar.add(trc::Material{ .color=vec4(1, 1, 0, 0.3f) });
    trc::Drawable cursor({ ar.add(trc::makeSphereGeo(16, 8)), cursorCubeMat, true, false }, scene);
    cursor.scale(0.15f);

    // Text
    trc::Font font = ar.getFonts().makeFont(TRC_TEST_FONT_DIR"/gil.ttf", 64);
    trc::Text text{ instance, font };
    text.rotateY(0.5f).translate(-1.3f, 0.0f, -0.1f);
    text.print("Hello World!");
    text.attachToScene(scene);

    trc::Timer timer;
    trc::Timer frameTimer;
    uint32_t frames{ 0 };
    while (running)
    {
        trc::pollEvents();

        const float frameTime = frameTimer.reset();
        scene.update(frameTime);
        cursor.setTranslation(torch->getRenderConfig().getMouseWorldPos(camera));

        torch->drawFrame(torch->makeDrawConfig(scene, camera));

        frames++;
        if (timer.duration() >= 1000)
        {
            std::cout << frames << " frames in one second\n";
            frames = 0;
            timer.reset();
        }
    }

    device->waitIdle();

    particleUpdateThread.join();
    particleSpawnThread.join();
    cubeRotateThread.join();
}

int main()
{
    // A function for extra scope
    run();

    trc::terminate();

    std::cout << " --- Done\n";
    return 0;
}
