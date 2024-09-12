#pragma once

#include <concepts>
#include <unordered_map>

#include <componentlib/Table.h>
#include <trc_util/Assert.h>

#include "trc/LightRegistry.h"
#include "trc/ShadowRegistry.h"
#include "trc/Types.h"
#include "trc/core/SceneModule.h"

namespace trc
{
    class Camera;

    class LightSceneModule : public SceneModule
                           , public LightRegistry
    {
    public:
        struct SunShadow
        {
        public:
            auto getCamera() -> Camera& {
                return *camera;
            }
            auto getCamera() const -> const Camera& {
                return *camera;
            }

        private:
            friend LightSceneModule;
            SunShadow(const s_ptr<Camera>& camera, ShadowID id)
                : camera(camera), shadowMapId(id) {
                assert_arg(camera != nullptr);
            }

            s_ptr<Camera> camera;
            ShadowID shadowMapId;
        };

        /**
         * @brief Enable shadows for a sun light
         *
         * @param Light light The light that shall cast shadows.
         * @param uvec2 shadowMapResolution
         *
         * @return A handle to the shadow. Does not need to be kept alive;
         *         destroy a shadow with `LightSceneModule::disableShadow`.
         */
        auto enableShadow(const SunLight& light,
                          uvec2 shadowMapResolution)
            -> s_ptr<SunShadow>;

        /**
         * @brief Access a light's shadow.
         *
         * @return nullptr if shadows are not enabled for the light. The light's
         *         shadow otherwise.
         */
        auto getShadow(const SunLight& light) -> s_ptr<SunShadow>;

        /**
         * @brief Disable shadows for a light.
         *
         * Does nothing if shadows are not enabled for the light.
         */
        void disableShadow(const SunLight& light);

        /**
         * @return Handle to the created listener. Removes the listener from the
         *         light scene when destroyed.
         */
        template<std::invocable<const ShadowRegistry::ShadowCreateEvent&> F>
        [[nodiscard]]
        auto onShadowCreate(F&& func) -> EventListener<ShadowRegistry::ShadowCreateEvent> {
            return shadowRegistry.onShadowCreate(std::forward<F>(func));
        }

        /**
         * @return Handle to the created listener. Removes the listener from the
         *         light scene when destroyed.
         */
        template<std::invocable<const ShadowRegistry::ShadowDestroyEvent&> F>
        [[nodiscard]]
        auto onShadowDestroy(F&& func) -> EventListener<ShadowRegistry::ShadowDestroyEvent> {
            return shadowRegistry.onShadowDestroy(std::forward<F>(func));
        }

    private:
        ShadowRegistry shadowRegistry;
        std::unordered_map<SunLight, s_ptr<SunShadow>> sunShadows;
    };

    using SunShadow = s_ptr<LightSceneModule::SunShadow>;
} // namespace trc
