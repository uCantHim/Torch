#include <chrono>
using namespace std::chrono;
#include <iostream>
#include <fstream>
#include <thread>

#include <glm/gtc/random.hpp>

#include <vkb/Device.h>
#include <vkb/Buffer.h>
#include <vkb/MemoryPool.h>
#include <vkb/ImageUtils.h>

#include <trc/Torch.h>
using namespace trc::basic_types;
#include <trc/particle/Particle.h>
#include <trc/asset_import/FBXLoader.h>
#include <trc/asset_import/AssetUtils.h>
#include <trc/text/Text.h>

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
    auto& ar = torch.renderConfig->getAssets();
    auto& instance = *torch.instance;
    const auto& device = instance.getDevice();

    // ------------------
    // Random test things

    trc::FBXLoader fbxLoader;
    auto grassImport = fbxLoader.loadFBXFile(TRC_TEST_ASSET_DIR"/grass_lowpoly.fbx");
    auto treeImport = fbxLoader.loadFBXFile(TRC_TEST_ASSET_DIR"/tree_lowpoly.fbx");
    auto mapImport = fbxLoader.loadFBXFile(TRC_TEST_ASSET_DIR"/map.fbx");

    auto grassGeoIndex = ar.add(grassImport.meshes[0].mesh);
    auto treeGeoIndex = ar.add(treeImport.meshes[0].mesh);
    auto mapGeoIndex = ar.add(mapImport.meshes[0].mesh);

    auto mapMatIndex = ar.add(mapImport.meshes[0].materials[0]);

    auto skeletonGeoIndex = trc::loadGeometry(TRC_TEST_ASSET_DIR"/skeleton.fbx", ar).get();
    auto hoodedBoiGeoIndex = trc::loadGeometry(TRC_TEST_ASSET_DIR"/hooded_boi.fbx", ar).get();
    auto lindaMesh = fbxLoader.loadFBXFile(TRC_TEST_ASSET_DIR"/Female_Character.fbx").meshes[0];
    auto lindaGeoIndex = ar.add(lindaMesh.mesh, lindaMesh.rig);

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
    trc::DrawablePool pool(instance, { .maxInstances=1000, .initRayTracing=false }, scene);

    std::vector<trc::AnimationEngine*> animEngines;

    auto grass = pool.create({ grassGeoIndex, matIdx });
    grass->setScale(0.1f).rotateX(glm::radians(-90.0f)).translateX(0.5f);

    // Animated skeleton
    auto skeleton = pool.create({ skeletonGeoIndex, matIdx });
    skeleton->scale(0.02f).translateZ(1.2f)
            .translate(1.0f, 0.0f, 0.0f)
            .rotateY(-glm::half_pi<float>());
    animEngines.emplace_back(&skeleton->getAnimationEngine())->playAnimation(0);

    for (int i = 1; i < 50; i++)
    {
        auto inst = skeleton->copy();
        const float angle = glm::two_pi<float>() / 50 * i;
        inst->scale(0.02f).translateZ(1.2f)
            .translate(glm::cos(angle), 0.0f, glm::sin(angle))
            .rotateY(-glm::half_pi<float>() - angle);
        animEngines.emplace_back(&inst->getAnimationEngine())->playAnimation(0);
    }

    // Hooded boi
    auto hoodedBoi = pool.create({ hoodedBoiGeoIndex, {} });
    hoodedBoi->setScale(0.2f).translate(1.0f, 0.6f, -7.0f);
    animEngines.emplace_back(&hoodedBoi->getAnimationEngine())->playAnimation(0);

    // Linda
    auto lindaMatIdx = ar.add({
        .kSpecular = vec4(0.0f),
        .diffuseTexture = lindaDiffTexIdx
    });

    auto linda = pool.create({ lindaGeoIndex, lindaMatIdx });
    linda->setScale(0.3f).translateX(-1.0f);
    animEngines.emplace_back(&linda->getAnimationEngine())->playAnimation(0);

    // Generated plane geo
    auto myPlaneGeoIndex = ar.add(trc::makePlaneGeo(20.0f, 20.0f, 20, 20));
    auto myPlane = pool.create({ myPlaneGeoIndex, mapMatIndex });

    trc::Light sunLight = scene.getLights().makeSunLight(vec3(1.0f), vec3(1.0f, -1.0f, -1.5f));
    trc::Light ambientLight = scene.getLights().makeAmbientLight(vec3(0.15f));
    trc::Light pointLight = scene.getLights().makePointLight(vec3(1, 1, 0), vec3(2, 0.5f, 0.5f), 0.4f);

    // Sun light
    mat4 proj = glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, -50.0f, 5.0f);
    auto& shadowNode = scene.enableShadow(
        sunLight,
        { .shadowMapResolution=uvec2(2048, 2048) },
        *torch.shadowPool
    );
    shadowNode.setProjectionMatrix(proj);
    scene.getRoot().attach(shadowNode);

    // Instanced trees
    constexpr trc::ui32 NUM_TREES = 800;
    auto firstTree = pool.create({ treeGeoIndex, treeMatIdx });
    firstTree->setScale(0.1f).rotateX(glm::radians(-90.0f))
        .setTranslation(-3.0f, 0.0f, -1.0f);
    for (int i = 1; i < NUM_TREES; i++)
    {
        firstTree->copy()
            ->setScale(0.1f).rotateX(glm::radians(-90.0f))
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
        vkb::Timer timer;
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


    // Custom cube geo test
    auto cubeGeoIdx = ar.add({ trc::makeCubeGeo() });
    auto cubeMatIdx = ar.add({ .color={ 0.3, 0.3, 1, 0.5} });
    auto cube = pool.create({ cubeGeoIdx, cubeMatIdx, true });
    cube->translate(1.5f, 0.7f, 1.5f).setScale(0.3f);
    std::thread cubeRotateThread([&cube, &running]() {
        while (running)
        {
            std::this_thread::sleep_for(10ms);
            cube->rotateY(glm::radians(0.5f));
        }
    });

    // Text
    trc::Font font = torch.assetRegistry->getFonts().makeFont(TRC_TEST_FONT_DIR"/gil.ttf", 64);
    trc::Text text{ instance, font };
    text.rotateY(0.5f).translate(-1.3f, 0.0f, -0.1f);
    text.print("Hello World!");
    text.attachToScene(scene);

    vkb::Timer timer;
    vkb::Timer animTimer;
    uint32_t frames{ 0 };
    while (running)
    {
        trc::pollEvents();

        scene.updateTransforms();

        const float time = animTimer.reset();
        for (auto anim : animEngines) {
            anim->update(time);
        }

        torch.drawFrame(torch.makeDrawConfig(scene, camera));

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
