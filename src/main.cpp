#include <chrono>
using namespace std::chrono;
#include <iostream>
#include <fstream>

#include <GLFW/glfw3.h>
#include <vkb/VulkanBase.h>
#include <vkb/Buffer.h>
#include <vkb/MemoryPool.h>

#include "trc/utils/FBXLoader.h"
#include "trc/Geometry.h"
#include "trc/AssetRegistry.h"
#include "trc/Drawable.h"
#include "trc/Scene.h"
#include "trc/Renderer.h"

trc::Camera camera({ { 0, 0 }, { 1, 1 } }, 45.0f, { 0.1f, 100.0f });

struct CameraResize : public vkb::SwapchainDependentResource
{
public:
    void signalRecreateRequired() override {}
    void recreate(vkb::Swapchain& sc) override {
        std::cout << "Window resized\n";
        camera.setViewport({ { 0, 0 }, { sc.getImageExtent().width, sc.getImageExtent().height } });
    }
    void signalRecreateFinished() override {}
};
static CameraResize _camera_resize_helper;

void resizeCameraViewport(GLFWwindow*, int sizeX, int sizeY)
{
}

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
    auto mapImport = fbxLoader.loadFBXFile("map.fbx");
    auto& grassGeo = trc::AssetRegistry::addGeometry(1, trc::Geometry(grassImport.meshes[0].mesh));
    auto& treeGeo = trc::AssetRegistry::addGeometry(2, trc::Geometry(treeImport.meshes[0].mesh));
    auto& mapGeo = trc::AssetRegistry::addGeometry(3, trc::Geometry(mapImport.meshes[0].mesh));

    trc::ui32 mat = 0;
    trc::AssetRegistry::addMaterial(mat, trc::Material());

    // ------------------

    const auto& swapchain = vkb::VulkanBase::getSwapchain();
    const auto& windowSize = swapchain.getImageExtent();

    trc::Scene scene;
    camera.setPosition({ 0, 2.0f, 5.0f });
    camera.setForwardVector({ 0, -2.0f / 5.0f, -1 });

    //trc::Drawable grass(grassGeo, mat, scene);
    //grass.setScale(0.1f).rotateX(glm::radians(-90.0f)).translateX(0.5f);

    //trc::Drawable tree(treeGeo, mat, scene);
    //tree.setScale(0.1f).rotateX(glm::radians(-90.0f)).translate(0, 0, -1.0f).rotateY(0.3f);

    //trc::Node node;
    //node.rotateX(-glm::radians(90.0f));
    //trc::Drawable map(mapGeo, mat, scene);
    //node.attach(map);
    //scene.getRoot().attach(node);

    trc::Light sunLight = trc::makeSunLight(vec3(1.0f), vec3(1.0f, 1.0f, -1.0f));
    trc::Light ambientLight = trc::makeAmbientLight(vec3(0.15f));
    trc::Light pointLight = trc::makePointLight(vec3(1, 1, 0), vec3(0, 1, 1), 0.2f);
    scene.addLight(sunLight);
    scene.addLight(ambientLight);
    scene.addLight(pointLight);

    std::vector<trc::Drawable> trees;
    trees.reserve(800);
    for (int i = 0; i < 800; i++)
    {
        auto& d = trees.emplace_back(treeGeo, mat, scene);
        d.setScale(0.1f).rotateX(glm::radians(-90.0f));
        d.setTranslationX(-3.0f + static_cast<float>(i % 14) * 0.5f);
        d.setTranslationZ(-1.0f - (static_cast<float>(i) / 14.0f) * 0.4f);
    }

    while (true)
    {
        auto start = system_clock::now();

        renderer.drawFrame(scene, camera);

        auto end = system_clock::now();
        auto duration = duration_cast<microseconds>(end - start).count();
        std::cout << "Frame duration: " << duration << " µs\n";
    }

    std::cout << " --- Done\n";
    return 0;
}
