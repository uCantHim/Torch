#include "trc/LightSceneModule.h"



namespace trc
{

auto LightSceneModule::enableShadow(
    Light light,
    const ShadowCreateInfo& shadowInfo,
    ShadowPool& shadowPool
    ) -> ShadowNode&
{
    if (light.getType() != Light::Type::eSunLight) {
        throw std::invalid_argument("Shadows are currently only supported for sun lights");
    }
    if (lightExists(light)) {
        throw std::invalid_argument("Light does not exist in the light registry!");
    }
    if (shadowNodes.find(light) != shadowNodes.end()) {
        throw std::invalid_argument("Shadows are already enabled for the light!");
    }

    auto [it, success] = shadowNodes.try_emplace(light, std::make_unique<ShadowNode>());
    if (!success) {
        throw std::runtime_error("Unable to add light to the map in LightRegistry::enableShadow");
    }

    auto& newNode = *it->second;
    for (ui32 i = 0; i < getNumShadowMaps(light.getType()); i++)
    {
        ShadowMap& shadow = newNode.shadows.emplace_back(
            shadowPool.allocateShadow(shadowInfo)
        );
        newNode.attach(*shadow.camera);

        // Add shadow to light info
        light.addShadowMap(shadow.index);

        // Add the dynamic render pass
        shadowPasses.emplace(shadow.renderPass);

        // Use lookAt for sun lights
        if (light.getType() == LightData::Type::eSunLight && length(light.getDirection()) > 0.0f)
        {
            shadow.camera->lookAt(vec3(0.0f), light.getDirection(), vec3(0, 1, 0));
        }
    }

    newNode.update();

    return newNode;
}

void LightSceneModule::disableShadow(Light light)
{
    auto it = shadowNodes.find(light);
    if (it != shadowNodes.end())
    {
        light.removeAllShadowMaps();

        // Remove all render passes
        for (auto& shadow : it->second->shadows) {
            shadowPasses.erase(shadow.renderPass);
        }

        shadowNodes.erase(it);
    }
}

} // namespace trc
