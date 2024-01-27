#pragma once

#include <componentlib/Table.h>

#include "trc/LightRegistry.h"
#include "trc/Node.h"
#include "trc/ShadowPool.h"
#include "trc/Types.h"
#include "trc/core/SceneModule.h"

namespace trc
{
    class LightSceneModule : public SceneModule
                           , public LightRegistry
    {
    public:
        /**
         * @brief Handle to a light's shadow
         */
        struct ShadowNode : public Node
        {
            /**
             * @brief Set a projection matrix on all shadow cameras
             */
            void setProjectionMatrix(mat4 proj) noexcept
            {
                for (auto& shadow : shadows) {
                    shadow.camera->setProjectionMatrix(proj);
                }
            }

        private:
            friend LightSceneModule;

            std::vector<ShadowMap> shadows;
        };

        /**
         * @brief Enable shadows for a specific light
         *
         * @param Light light The light that shall cast shadows.
         * @param const ShadowCreateInfo& shadowInfo
         * @param ShadowPool& shadowPool A shadow pool from which to
         *        allocate shadow maps.
         *
         * @return ShadowNode& The node is NOT automatically attached to
         *                     the scene's root.
         *
         * @throw std::invalid_argument if shadows are already enabled on the light
         * @throw std::runtime_error if something unexpected happens
         */
        auto enableShadow(Light light,
                          const ShadowCreateInfo& shadowInfo,
                          ShadowPool& shadowPool) -> ShadowNode&;

        /**
         * Does nothing if shadows are not enabled for the light
         */
        void disableShadow(Light light);

        auto getShadowPasses() -> const std::unordered_set<s_ptr<RenderPass>>& {
            return shadowPasses;
        }

    private:
        std::unordered_map<Light, u_ptr<ShadowNode>> shadowNodes;
        std::unordered_set<s_ptr<RenderPass>> shadowPasses;
    };
} // namespace trc
