#include <chrono>
using namespace std::chrono;
#include <iostream>
#include <fstream>

#include <vkb/VulkanBase.h>
#include <vkb/Buffer.h>
#include <vkb/MemoryPool.h>

#include "trc/utils/FBXLoader.h"
#include "trc/Geometry.h"
#include "trc/AssetRegistry.h"
#include "trc/Drawable.h"
#include "trc/Scene.h"
#include "trc/Renderer.h"

int main()
{
    using v = vkb::VulkanBase;

    vkb::VulkanInitInfo initInfo;
    vkb::vulkanInit(initInfo);

    trc::Renderer renderer;

    // ------------------
    // Random test things

    trc::FBXLoader fbxLoader;
    auto grassImport = fbxLoader.loadFBXFile("grass_lowpoly.fbx");
    auto treeImport = fbxLoader.loadFBXFile("tree_lowpoly.fbx");
    auto& grassGeo = trc::AssetRegistry::add(1, trc::Geometry(grassImport.meshes[0].mesh));
    auto& treeGeo = trc::AssetRegistry::add(2, trc::Geometry(treeImport.meshes[0].mesh));
    auto& mat = trc::AssetRegistry::add(0, trc::Material());

    vkb::MemoryPool pool(v::getDevice().getPhysicalDevice(), 20000);
    vkb::Buffer pooledBuf(500, vk::BufferUsageFlagBits::eStorageBuffer, {}, pool.makeAllocator());

    // ------------------

    const auto& swapchain = vkb::VulkanBase::getSwapchain();
    const auto& windowSize = swapchain.getImageExtent();
    constexpr size_t NUM_OBJECTS = 2000;

    trc::Scene scene;
    trc::Camera camera({ { 0, 0 }, { windowSize.width, windowSize.height } }, 45.0f, { 0.1f, 100.0f });
    camera.setPosition({ 1, 0, 1 });
    camera.setForwardVector({ -1, 0, -1 });

    trc::Drawable grass(grassGeo, mat);
    grass.attachToScene(scene);
    grass.setScale(0.2f).rotateX(glm::quarter_pi<float>()).translateX(-0.5);

    trc::Drawable tree(treeGeo, mat);
    tree.attachToScene(scene);
    tree.setScale(0.04f).rotateX(glm::quarter_pi<float>());

    while (true)
    {
        renderer.drawFrame(scene, camera);
    }

    std::cout << " --- Done\n";
    return 0;
}
