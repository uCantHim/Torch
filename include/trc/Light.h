#pragma once

#include "trc/Types.h"

namespace trc
{
    class LightRegistry;

    /**
     * @brief Internal light data for usage on the device
     */
    struct LightDeviceData
    {
        enum Type : ui32
        {
            eSunLight = 0,
            ePointLight = 1,
            eAmbientLight = 2,
        };

        vec4 color{ 1.0f };
        vec4 position{ 0.0f };
        vec4 direction{ 1.0f };

        float ambientPercentage{ 0.0f };
        float attenuationLinear{ 0.5f };
        float attenuationQuadratic{ 0.0f };

        Type type;

        static constexpr ui32 MAX_SHADOW_MAPS{ 4 };

        ui32 numShadowMaps{ 0 };
        ui32 shadowMapIndices[MAX_SHADOW_MAPS]{ 0 };

        ui32 __padding[3]{ 0 };
    };

    namespace impl
    {
        class LightInterfaceBase
        {
        public:
            explicit LightInterfaceBase(LightDeviceData* data) : data(data) {
                assert(this->data != nullptr);
            }

            /**
             * Link a shadow map to the light.
             *
             * @return True if the shadow map was successfully linked to the
             *         light, false if it exceeded the maximum allowed number
             *         of shadow maps per light.
             */
            auto linkShadowMap(ui32 shadowMapIndex) -> bool;

            /**
             * Remove all linked shadow maps from the light.
             */
            void clearShadowMaps();

        protected:
            LightDeviceData* data;
        };

        class ColoredLightInterface : public LightInterfaceBase
        {
        public:
            auto getColor() const -> vec3;
            auto getAmbientPercentage() const -> float;

            void setColor(vec3 newColor);

            /**
             * @brief Set how much the light's color is added as ambient lighting
             *
             * A convenient and cheap way to fake global illumination effects.
             *
             * @param ambient The new ambient light percentage. Should be in the
             *                range [0, 1].
             */
            void setAmbientPercentage(float ambient);
        };
    } // namespace impl

    class SunLightInterface : public impl::ColoredLightInterface
    {
    public:
        auto getDirection() const -> vec3;
        void setDirection(vec3 newDir);
    };

    class PointLightInterface : public impl::ColoredLightInterface
    {
    public:
        auto getPosition() const -> vec3;
        void setPosition(vec3 newPos);
    };

    class AmbientLightInterface : public impl::ColoredLightInterface
    {
    };
} // namespace trc
