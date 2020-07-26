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
#include "trc/DrawableInstanced.h"
#include "trc/Scene.h"
#include "trc/Renderer.h"

trc::Camera camera({ { 0, 0 }, { 1, 1 } }, 45.0f, { 0.1f, 100.0f });

struct CameraResize : public vkb::SwapchainDependentResource
{
public:
    void signalRecreateRequired() override {}
    void recreate(vkb::Swapchain& sc) override {
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

    auto [img, imgIndex] = trc::AssetRegistry::addImage(
        vkb::Image("/home/nicola/dotfiles/arch_3D_simplistic.png")
    );

    auto [stoneTex, stoneTexIdx] = trc::AssetRegistry::addImage(
        vkb::Image("assets/rough_stone_wall.tif")
    );
    auto [stoneNormalTex, stoneNormalTexIdx] = trc::AssetRegistry::addImage(
        vkb::Image("assets/rough_stone_wall_normal.tif")
    );

    auto [mat, matIdx] = trc::AssetRegistry::addMaterial(trc::Material());
    mat.get().colorAmbient = vec4(1.0f);
    mat.get().colorDiffuse = vec4(1.0f);
    mat.get().colorSpecular = vec4(1.0f);
    mat.get().diffuseTexture = stoneTexIdx;
    mat.get().bumpTexture = stoneNormalTexIdx;
    mat.get().shininess = 2.0f;

    trc::AssetRegistry::updateMaterialBuffer();

    // Additional textures
    trc::AssetRegistry::addImage(vkb::Image("assets/jute_normal.tif"));

    // ------------------

    trc::Renderer renderer;

    const auto& swapchain = vkb::VulkanBase::getSwapchain();
    const auto& windowSize = swapchain.getImageExtent();

    trc::Scene scene;
    camera.setPosition({ 0, 2.0f, 5.0f });
    camera.setForwardVector({ 0, -2.0f / 5.0f, -1 });

    trc::Drawable grass(grassGeo, matIdx, scene);
    grass.setScale(0.1f).rotateX(glm::radians(-90.0f)).translateX(0.5f);

    trc::Drawable tree(treeGeo, matIdx, scene);
    tree.setScale(0.1f).rotateX(glm::radians(-90.0f)).translate(0, 0, -1.0f).rotateY(0.3f);

    //trc::Node node;
    //node.rotateX(-glm::radians(90.0f));
    //trc::Drawable map(mapGeo, matIdx, scene);
    //node.attach(map);
    //scene.getRoot().attach(node);

    auto planeImport = fbxLoader.loadFBXFile("assets/plane.fbx");
    auto [planeGeo, planeGeoIndex] = trc::AssetRegistry::addGeometry(trc::Geometry(planeImport.meshes[0].mesh));
    trc::Drawable plane(planeGeo, matIdx, scene);
    plane.rotateY(glm::radians(-65.0f));
    plane.translate(0.5f, 0.7f, 1.0f);

    trc::Light sunLight = trc::makeSunLight(vec3(1.0f), vec3(1.0f, -1.0f, -1.0f));
    trc::Light ambientLight = trc::makeAmbientLight(vec3(0.15f));
    trc::Light pointLight = trc::makePointLight(vec3(1, 1, 0), vec3(0, 1, 1), 0.2f);
    scene.addLight(sunLight);
    scene.addLight(ambientLight);
    //scene.addLight(pointLight);

    // Instanced trees
    constexpr trc::ui32 NUM_TREES = 800;

    trc::DrawableInstanced instancedTrees(NUM_TREES, treeGeo, scene);
    for (int i = 0; i < NUM_TREES; i++)
    {
        trc::Transformation t;
        t.setScale(0.1f).rotateX(glm::radians(-90.0f));
        t.setTranslationX(-3.0f + static_cast<float>(i % 14) * 0.5f);
        t.setTranslationZ(-1.0f - (static_cast<float>(i) / 14.0f) * 0.4f);

        instancedTrees.addInstance({ t.getTransformationMatrix(), matIdx });
    }


    while (true)
    {
        renderer.drawFrame(scene, camera);
    }

    std::cout << " --- Done\n";
    return 0;
}
