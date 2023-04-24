#include "trc/Scene.h"

#include "trc/TorchRenderStages.h"



trc::Scene::Scene()
    :
    SceneBase(),
    DrawableComponentScene(static_cast<SceneBase&>(*this))
{
}

void trc::Scene::update(const float timeDelta)
{
    // Update transformations in the node tree
    root.updateAsRoot();

    DrawableComponentScene::updateAnimations(timeDelta);
    DrawableComponentScene::updateRayData();
}

auto trc::Scene::getRoot() noexcept -> Node&
{
    return root;
}

auto trc::Scene::getRoot() const noexcept -> const Node&
{
    return root;
}

auto trc::Scene::getLights() noexcept -> LightRegistry&
{
    return lightRegistry;
}

auto trc::Scene::getLights() const noexcept -> const LightRegistry&
{
    return lightRegistry;
}

auto trc::Scene::enableShadow(
    Light light,
    const ShadowCreateInfo& shadowInfo,
    ShadowPool& shadowPool
    ) -> ShadowNode&
{
    if (light.getType() != Light::Type::eSunLight) {
        throw std::invalid_argument("Shadows are currently only supported for sun lights");
    }
    if (lightRegistry.lightExists(light)) {
        throw std::invalid_argument("Light does not exist in the light registry!");
    }
    if (shadowNodes.find(light) != shadowNodes.end()) {
        throw std::invalid_argument("Shadows are already enabled for the light!");
    }

    auto [it, success] = shadowNodes.try_emplace(light);
    if (!success) {
        throw std::runtime_error("Unable to add light to the map in LightRegistry::enableShadow");
    }

    auto& newEntry = it->second;
    for (ui32 i = 0; i < getNumShadowMaps(light.getType()); i++)
    {
        ShadowMap& shadow = newEntry.shadows.emplace_back(
            shadowPool.allocateShadow(shadowInfo)
        );
        Camera& camera = *shadow.camera;
        newEntry.attach(camera);

        // Add shadow to light info
        light.addShadowMap(shadow.index);

        // Add the dynamic render pass
        addRenderPass(shadowRenderStage, *shadow.renderPass);

        // Use lookAt for sun lights
        if (light.getType() == LightData::Type::eSunLight && length(light.getDirection()) > 0.0f)
        {
            camera.lookAt(vec3(0.0f), light.getDirection(), vec3(0, 1, 0));
        }
    }

    newEntry.update();

    return newEntry;
}

void trc::Scene::disableShadow(Light light)
{
    auto it = shadowNodes.find(light);
    if (it != shadowNodes.end())
    {
        light.removeAllShadowMaps();

        // Remove all render passes
        for (auto& shadow : it->second.shadows) {
            removeRenderPass(shadowRenderStage, *shadow.renderPass);
        }

        shadowNodes.erase(it);
    }
}
