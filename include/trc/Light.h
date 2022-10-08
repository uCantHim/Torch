#pragma once

#include "trc/Types.h"
#include "trc_util/Padding.h"

namespace trc
{
    /**
     * @brief Internal light data for usage on the device
     */
    struct LightData
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

    static_assert(sizeof(LightData) == util::sizeof_pad_16_v<LightData>,
                  "LightData structs must be padded to a Device-friendly length");

    /**
     * @brief Light handle
     *
     * TODO: Implement typesafe handles for different light types
     */
    class Light
    {
    public:
        using Type = LightData::Type;

        constexpr Light() = default;

        auto operator<=>(const Light&) const = default;

        /**
         * @return bool True if the object is a valid handle to an existing
         *              light, false otherwise.
         */
        operator bool() const;

        auto getType() const -> Type;
        auto getColor() const -> vec3;
        auto getPosition() const -> vec3;
        auto getDirection() const -> vec3;
        auto getAmbientPercentage() const -> float;

        void setColor(vec3 newColor);

        /**
         * Only affects point lights
         */
        void setPosition(vec3 newPos);

        /**
         * Only affects sun lights
         */
        void setDirection(vec3 newDir);

        /**
         * @brief Set how much the light's color is added as ambient lighting
         */
        void setAmbientPercentage(float ambient);

        void addShadowMap(ui32 shadowMapIndex);
        void removeAllShadowMaps();

    private:
        friend struct std::hash<trc::Light>;
        friend class LightRegistry;
        explicit Light(LightData& lightData);

        LightData* data{ nullptr };
    };

    /**
     * @brief Unique light handle
     */
    using UniqueLight = u_ptr<Light, std::function<void(Light*)>>;

    static_assert(std::regular<Light>, "Light handles are regular types");

    /**
     * @return Number of shadow maps based on light type
     * @throw std::logic_error if the enum doesn't exist
     */
    extern ui32 getNumShadowMaps(Light::Type lightType);
} // namespace trc



namespace std
{
    /**
     * @brief std::hash specialization for Light handles
     */
    template<>
    struct hash<trc::Light>
    {
        size_t operator()(const trc::Light& light) const noexcept {
            return hash<trc::LightData*>{}(light.data);
        }
    };
} // namespace std
