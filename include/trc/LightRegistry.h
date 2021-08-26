#pragma once

#include <vkb/Buffer.h>

#include "Light.h"

namespace trc
{
    class Instance;

    /**
     * @brief Collection and management unit for lights and shadows
     */
    class LightRegistry
    {
    public:
        LightRegistry() = default;

        /**
         * @brief Create a sunlight
         */
        auto makeSunLight(vec3 color,
                          vec3 direction,
                          float ambientPercent = 0.0f) -> Light;

        /**
         * @brief Create a sunlight
         */
        auto makeSunLightUnique(vec3 color,
                                vec3 direction,
                                float ambientPercent = 0.0f) -> UniqueLight;

        /**
         * @brief Create a pointlight
         */
        auto makePointLight(vec3 color,
                            vec3 position,
                            float attLinear = 0.0f,
                            float attQuadratic = 0.0f) -> Light;

        /**
         * @brief Create a pointlight
         */
        auto makePointLightUnique(vec3 color,
                                  vec3 position,
                                  float attLinear = 0.0f,
                                  float attQuadratic = 0.0f) -> UniqueLight;

        /**
         * @brief Create an ambient light
         */
        auto makeAmbientLight(vec3 color) -> Light;

        /**
         * @brief Create an ambient light
         */
        auto makeAmbientLightUnique(vec3 color) -> UniqueLight;

        /**
         * @return bool True if the light exists in the registry, false
         *              otherwise.
         */
        bool lightExists(Light light);

        /**
         * Also removes a light node that has the light attached if such
         * a node exists.
         */
        void deleteLight(Light light);

        auto getRequiredLightDataSize() const -> ui32;
        void writeLightData(ui8* buf) const;

    private:
        /**
         * @return const Light& Handle to the new light
         */
        auto addLight(LightData light) -> Light;

        /**
         * I could keep all lights in one array instead and sort that array
         * by light type before every buffer update. But I wanted to try
         * something different and see how it works out.
         */
        std::vector<u_ptr<LightData>> sunLights;
        std::vector<u_ptr<LightData>> pointLights;
        std::vector<u_ptr<LightData>> ambientLights;

        ui32 requiredLightDataSize { 0 };
    };
} // namespace trc
