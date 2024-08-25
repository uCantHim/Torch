#include "trc/LightSceneModule.h"

#include "trc/Camera.h"



namespace trc
{

auto LightSceneModule::enableShadow(
    const SunLight& light,
    uvec2 shadowMapResolution
    ) -> s_ptr<SunShadow>
{
    auto it = sunShadows.find(light);
    if (it != sunShadows.end()) {
        return it->second;
    }

    // Create a shadow camera
    auto camera = std::make_shared<Camera>();
    camera->lookAt(vec3(0.0f), light->getDirection(), vec3(0, 1, 0));

    // Create the shadow
    const ShadowID id = shadowRegistry.makeShadow({ shadowMapResolution, camera });
    s_ptr<SunShadow> shadow{ new SunShadow{ camera, id } };
    sunShadows.try_emplace(light, shadow);

    // Link shadow to the light
    light->linkShadowMap(id);

    return shadow;
}

void LightSceneModule::disableShadow(const SunLight& light)
{
    auto node = sunShadows.extract(light);

    // Free the shadow
    shadowRegistry.freeShadow(node.mapped()->shadowMapId);
    light->clearShadowMaps();
}

} // namespace trc
