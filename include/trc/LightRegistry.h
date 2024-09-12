#pragma once

#include <cassert>

#include <componentlib/Table.h>
#include <trc_util/Padding.h>
#include <trc_util/data/IdPool.h>

#include "trc/Types.h"
#include "trc/Light.h"

namespace trc
{
    template<std::derived_from<impl::LightInterfaceBase> T>
    using LightHandle = s_ptr<T>;

    using SunLight = LightHandle<SunLightInterface>;
    using PointLight = LightHandle<PointLightInterface>;
    using AmbientLight = LightHandle<AmbientLightInterface>;

    /**
     * @brief Collection and management unit for lights and shadows
     */
    class LightRegistry
    {
    public:
        LightRegistry(const LightRegistry&) = delete;
        LightRegistry(LightRegistry&&) noexcept = delete;
        LightRegistry& operator=(const LightRegistry&) = delete;
        LightRegistry& operator=(LightRegistry&&) noexcept = delete;

        LightRegistry() = default;
        ~LightRegistry() noexcept = default;

        /**
         * @brief Create a sunlight
         */
        [[nodiscard]]
        auto makeSunLight(vec3 color,
                          vec3 direction,
                          float ambientPercent = 0.0f) -> SunLight;

        /**
         * @brief Create a pointlight
         */
        [[nodiscard]]
        auto makePointLight(vec3 color,
                            vec3 position,
                            float attLinear = 0.0f,
                            float attQuadratic = 0.0f) -> PointLight;

        /**
         * @brief Create an ambient light
         */
        [[nodiscard]]
        auto makeAmbientLight(vec3 color) -> AmbientLight;

        /**
         * @return Required light buffer size in bytes.
         */
        auto getRequiredLightDataSize() const -> size_t;

        /**
         * @param buf Buffer to write to. Must be at least as large as the
         *            number of bytes returned by `getRequiredLightDataSize()`.
         */
        void writeLightData(ui8* buf) const;

    private:
        struct LightIdTypeTag {};
        using LightID = data::TypesafeID<LightIdTypeTag, ui32>;

        using LightTable = componentlib::Table<
            LightDeviceData,
            LightID,
            componentlib::StableTableImpl<LightDeviceData, LightID>
        >;

        class LightDataStorage
        {
        public:
            auto allocLight(const LightDeviceData& initData = {})
                -> std::pair<LightID, LightDeviceData*>
            {
                const auto id = LightID{lightIdPool.generate()};
                auto& data = lights.emplace(id, initData);
                ++numLights;
                return { id, &data };
            }

            void freeLight(LightID id)
            {
                --numLights;
                lights.erase(id);
                lightIdPool.free(id);
            }

            auto size() const -> size_t {
                return numLights;
            }

            auto begin() const { return lights.begin(); }
            auto end() const   { return lights.end(); }

        private:
            data::IdPool<LightID::IndexType> lightIdPool;
            LightTable lights;
            size_t numLights{ 0 };
        };

        /**
         * We store an s_ptr to the owning LightDataStorage in the LightHandle's
         * destructor, which also keeps the `data` pointer in the light handle
         * alive.
         */
        template<typename LightType>
        static auto makeLight(const s_ptr<LightDataStorage>& storage, const LightDeviceData& init)
            -> LightHandle<LightType>;

        static_assert(sizeof(LightDeviceData) == util::pad_16(sizeof(LightDeviceData)),
                      "LightData structs must be padded to a device-friendly length");

        static constexpr size_t kHeaderSize{ util::pad_16(3 * sizeof(ui32)) };
        static constexpr size_t kLightSize{ util::pad_16(sizeof(LightDeviceData)) };

        s_ptr<LightDataStorage> sunLights{ std::make_shared<LightDataStorage>() };
        s_ptr<LightDataStorage> pointLights{ std::make_shared<LightDataStorage>() };
        // LightDataStorage spotLights{ std::make_shared<LightDataStorage>() };
        s_ptr<LightDataStorage> ambientLights{ std::make_shared<LightDataStorage>() };

        size_t requiredLightDataSize { 0 };
    };
} // namespace trc
