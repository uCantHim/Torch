#include <chrono>
using namespace std::chrono;
#include <iostream>
#include <fstream>
#include <thread>

#include <glm/gtc/random.hpp>
#include <vkb/VulkanBase.h>
#include <vkb/Buffer.h>
#include <vkb/MemoryPool.h>
#include <vkb/event/Event.h>
#include <vkb/event/InputState.h>
using namespace glm;

#include "trc/utils/FBXLoader.h"
#include "trc/Geometry.h"
#include "trc/AssetRegistry.h"
#include "trc/Drawable.h"
#include "trc/DrawableInstanced.h"
#include "trc/Scene.h"
#include "trc/Renderer.h"
#include "trc/Particle.h"

#include "trc/AssetUtils.h"

int main()
{
    trc::Camera camera(1.0f, 45.0f, 0.1f, 100.0f);
    vkb::EventHandler<vkb::SwapchainResizeEvent>::addListener([&](const auto& e) {
        const auto extent = e.swapchain->getImageExtent();
        camera.setAspect(float(extent.width) / float(extent.height));
    });

    vkb::Keyboard::init();
    vkb::Mouse::init();

    vkb::VulkanInitInfo initInfo;
    vkb::vulkanInit(initInfo);

    // ------------------
    // Random test things

    trc::FBXLoader fbxLoader;
    auto grassImport = fbxLoader.loadFBXFile("assets/grass_lowpoly.fbx");
    auto treeImport = fbxLoader.loadFBXFile("assets/tree_lowpoly.fbx");
    auto mapImport = fbxLoader.loadFBXFile("assets/map.fbx");

    auto [grassGeo, grassGeoIndex] = trc::AssetRegistry::addGeometry(trc::Geometry(grassImport.meshes[0].mesh));
    auto [treeGeo, treeGeoIndex] = trc::AssetRegistry::addGeometry(trc::Geometry(treeImport.meshes[0].mesh));
    auto [mapGeo, mapGeoIndex] = trc::AssetRegistry::addGeometry(trc::Geometry(mapImport.meshes[0].mesh));

    auto [treeMat, treeMatIndex] = trc::AssetRegistry::addMaterial(treeImport.meshes[0].materials[0]);
    auto [mapMat, mapMatIndex] = trc::AssetRegistry::addMaterial(mapImport.meshes[0].materials[0]);

    auto skeletonImport = fbxLoader.loadFBXFile("assets/skeleton.fbx");
    auto [skeletonGeo, skeletonGeoIndex] = trc::AssetRegistry::addGeometry(
        trc::Geometry(
            skeletonImport.meshes[0].mesh,
            std::make_unique<trc::Rig>(
                skeletonImport.meshes[0].rig.value(),
                skeletonImport.meshes[0].animations
            )
        )
    );
    auto hoodedBoiImport = fbxLoader.loadFBXFile("assets/hooded_boi.fbx");
    auto [hoodedBoiGeo, hoodedBoiGeoIndex] = trc::AssetRegistry::addGeometry(
        {
            hoodedBoiImport.meshes[0].mesh,
            std::make_unique<trc::Rig>(
                hoodedBoiImport.meshes[0].rig.value(),
                hoodedBoiImport.meshes[0].animations
            )
        }
    );
    auto lindaMesh = fbxLoader.loadFBXFile("assets/Female_Character.fbx").meshes[0];
    auto [lindaGeo, lindaGeoIndex] = trc::AssetRegistry::addGeometry(
        trc::Geometry(
            lindaMesh.mesh,
            std::make_unique<trc::Rig>(lindaMesh.rig.value(), lindaMesh.animations)
        )
    );
    auto [lindaDiffTex, lindaDiffTexIdx] = trc::AssetRegistry::addImage(
        vkb::makeImage2D(vkb::getDevice(), "assets/Female_Character.png")
    );

    auto [img, imgIndex] = trc::AssetRegistry::addImage(
        vkb::makeImage2D(vkb::getDevice(), "/home/nicola/dotfiles/arch_3D_simplistic.png")
    );

    auto [grassImg, grassImgIdx] = trc::AssetRegistry::addImage(
        vkb::makeImage2D(vkb::getDevice(), "assets/grass_billboard_001.png")
    );
    auto [stoneTex, stoneTexIdx] = trc::AssetRegistry::addImage(
        vkb::makeImage2D(vkb::getDevice(), vec4(1.0f, 0.0f, 0.0f, 1.0f)) //"assets/rough_stone_wall.tif")
    );
    auto [stoneNormalTex, stoneNormalTexIdx] = trc::AssetRegistry::addImage(
        vkb::makeImage2D(vkb::getDevice(), "assets/rough_stone_wall_normal.tif")
    );

    auto [mat, matIdx] = trc::AssetRegistry::addMaterial(trc::Material());
    mat.get().colorAmbient = vec4(1.0f);
    mat.get().colorDiffuse = vec4(1.0f);
    mat.get().colorSpecular = vec4(1.0f);
    mat.get().diffuseTexture = grassImgIdx;
    mat.get().bumpTexture = stoneNormalTexIdx;
    mat.get().shininess = 2.0f;

    mapMat.get().colorAmbient = vec4(1.0f);
    mapMat.get().colorDiffuse = vec4(1.0f);
    mapMat.get().colorSpecular = vec4(1.0f);
    mapMat.get().diffuseTexture = stoneTexIdx;
    mapMat.get().bumpTexture = stoneNormalTexIdx;

    trc::AssetRegistry::updateMaterialBuffer();

    // ------------------

    auto renderer = std::make_unique<trc::Renderer>();

    auto scene = std::make_unique<trc::Scene>();
    camera.lookAt({ 0.0f, 2.0f, 5.0f }, vec3(0, 0.5f, -1.0f ), { 0, 1, 0 });

    trc::Drawable grass(grassGeo, matIdx, *scene);
    grass.setScale(0.1f).rotateX(glm::radians(-90.0f)).translateX(0.5f);

    // Animated skeleton
    std::vector<trc::Drawable> skeletons;
    skeletons.reserve(50);
    for (int i = 0; i < 50; i++)
    {
        auto& skeleton = skeletons.emplace_back(skeletonGeo, matIdx, *scene);
        skeleton.setScale(0.04f).translateX(-0.3f + 0.05f * i);
        skeleton.getAnimationEngine().playAnimation(0);
    }

    // Hooded boi
    trc::Drawable hoodedBoi(hoodedBoiGeo, treeMatIndex, *scene);
    hoodedBoi.setScale(0.2f).translate(1.0f, 0.6f, -7.0f);
    hoodedBoi.getAnimationEngine().playAnimation(0);

    // Linda
    auto [lindaMat, lindaMatIdx] = trc::AssetRegistry::addMaterial(lindaMesh.materials[0]);
    lindaMat.get().colorSpecular = vec4(0.0f);
    lindaMat.get().diffuseTexture = lindaDiffTexIdx;
    trc::AssetRegistry::updateMaterialBuffer();

    trc::Drawable linda(lindaGeo, matIdx, *scene);
    linda.enableTransparency();
    linda.setScale(0.3f).translateX(-1.0f);
    linda.getAnimationEngine().playAnimation(0);
    auto& pickable = linda.enablePicking<trc::PickableFunctional>(
        []() { std::cout << "Linda has been picked\n"; },
        []() { std::cout << "Linda is no longer picked\n"; }
    );

    // Custom plane geo
    auto [myPlaneGeo, myPlaneGeoIndex] = trc::AssetRegistry::addGeometry(
        trc::Geometry(trc::makePlaneGeo(20.0f, 20.0f, 20, 20))
    );
    trc::Drawable myPlane(myPlaneGeo, mapMatIndex, *scene);

    trc::Light sunLight = trc::makeSunLight(vec3(1.0f), vec3(1.0f, -1.0f, -1.0f));
    trc::Light ambientLight = trc::makeAmbientLight(vec3(0.15f));
    trc::Light pointLight = trc::makePointLight(vec3(1, 1, 0), vec3(2, 0.5f, 0.5f), 0.4f);
    scene->addLight(sunLight);
    scene->addLight(ambientLight);
    scene->addLight(pointLight);

    // Make shadow pass for sun light
    //mat4 proj = glm::perspective(glm::radians(45.0f), 1.0f, 1.0f, 50.0f);
    mat4 proj = glm::ortho(-3.0f, 3.0f, -3.0f, 3.0f, 1.0f, 30.0f);
    auto shadowPassNode = trc::enableShadow(sunLight, uvec2(2048, 2048), proj);
    shadowPassNode.setFromMatrix(glm::lookAt(vec3(-10, 10, 15), vec3(0, 0, 0), vec3(0, 1, 0)));
    scene->getRoot().attach(shadowPassNode);

    // Instanced trees
    constexpr trc::ui32 NUM_TREES = 800;

    auto instancedTrees = std::make_unique<trc::DrawableInstanced>(NUM_TREES, treeGeo, *scene);
    for (int i = 0; i < NUM_TREES; i++)
    {
        trc::Transformation t;
        t.setScale(0.1f).rotateX(glm::radians(-90.0f));
        t.setTranslationX(-3.0f + static_cast<float>(i % 14) * 0.5f);
        t.setTranslationZ(-1.0f - (static_cast<float>(i) / 14.0f) * 0.4f);

        instancedTrees->addInstance({ t.getTransformationMatrix(), mapMatIndex });
    }


    auto particleCollection = std::make_unique<trc::ParticleCollection>(10000);
    particleCollection->attachToScene(*scene);
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
        particleCollection->addParticle(particle);
    }

    auto [particleImg, particleImgIdx] = trc::AssetRegistry::addImage(
        vkb::makeImage2D(vkb::getDevice(), "assets/yellowlight.png"));
    trc::ParticleSpawn spawn(*particleCollection);
    for (int i = 0; i < 50; i++)
    {
        trc::Particle p;
        p.phys.linearVelocity = glm::linearRand(vec3(0.2, 0.2, 0.2), vec3(1.5f, 1.5f, 1.0f));
        p.phys.linearAcceleration = vec3(0, -2.0f, 0);
        p.phys.scaling = vec3(0.2f);
        p.phys.lifeTime = 3000.0f;
        p.material.texture = particleImgIdx;
        spawn.addParticle(p);
    }
    spawn.spawnParticles();



    auto kitchenScene = trc::loadScene("assets/transparency_test_scene.fbx");
    trc::Light _sunLight = trc::makeSunLight(vec3(1.0f), vec3(1.0f, -1.0f, -1.0f));
    trc::Light _pointLight = trc::makePointLight(vec3(1, 1, 0), vec3(2, 0.5f, 0.5f), 0.4f);
    //kitchenScene.scene.addLight(trc::makeSunLight(vec3(1.0f), vec3(1.0f, -1.0f, -1.0f)));
    //kitchenScene.scene.addLight(trc::makePointLight(vec3(1, 1, 0), vec3(2, 0.5f, 0.5f), 0.4f));
    kitchenScene.scene.addLight(_sunLight);
    kitchenScene.scene.addLight(_pointLight);

    trc::Camera kitchenCamera(1.0f, 45.0f, 0.1f, 100.0f);
    kitchenCamera.lookAt({ 10.0f, 5.0f, 10.0f }, vec3{ 0.0f }, { 0, 1, 0 });
    vkb::EventHandler<vkb::SwapchainResizeEvent>::addListener([&](const auto& e) {
        const auto extent = e.swapchain->getImageExtent();
        kitchenCamera.setAspect(float(extent.width) / float(extent.height));
    });



    bool running{ true };
    auto yo = vkb::EventHandler<vkb::SwapchainCloseEvent>::addListener(
        [&running](const vkb::SwapchainCloseEvent&) { running = false; }
    );

    std::thread particleUpdateThread([&]() {
        while (running) {
            particleCollection->update();
        }
    });
    std::thread particleSpawnThread([&]() {
        while (running)
        {
            spawn.spawnParticles();
            std::this_thread::sleep_for(2s);
        }
    });

    vkb::Timer timer;
    uint32_t frames{ 0 };
    while (running)
    {
        if (vkb::Keyboard::isPressed(vkb::Key::a)) std::cout << "A pressed\n";
        if (vkb::Keyboard::isPressed(vkb::Key::del)) std::cout << "DEL pressed\n";

        renderer->drawFrame(*scene, camera);

        trc::getMouseWorldPos(camera);

        vkb::pollEvents();
        frames++;

        if (timer.duration() >= 1000)
        {
            std::cout << frames << " frames in one second\n";
            frames = 0;
            timer.reset();
        }
    }

    vkb::getDevice()->waitIdle();
    particleUpdateThread.join();
    particleSpawnThread.join();
    particleCollection.reset();
    instancedTrees.reset();
    scene.reset();
    renderer.reset();
    trc::RenderStage::destroyAll();
    trc::RenderPass::destroyAll();
    trc::Pipeline::destroyAll();
    vkb::vulkanTerminate();

    std::cout << " --- Done\n";
    return 0;
}
