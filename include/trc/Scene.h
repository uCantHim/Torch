#pragma once

#include <vector>
#include <unordered_map>

#include "trc/LightRegistry.h"
#include "trc/Node.h"
#include "trc/ShadowPool.h"
#include "trc/Types.h"
#include "trc/core/SceneBase.h"
#include "trc/drawable/DrawableScene.h"

namespace trc
{
    class Scene : public SceneBase
                , public DrawableScene
    {
    public:
        Scene(const Scene&) = delete;
        Scene(Scene&&) noexcept = delete;
        auto operator=(const Scene&) -> Scene& = delete;
        auto operator=(Scene&&) noexcept -> Scene& = delete;

        Scene();
        ~Scene() = default;

        void update(float timeDelta);

        auto getRoot() noexcept -> Node&;
        auto getRoot() const noexcept -> const Node&;

        auto getLights() noexcept -> LightRegistry&;
        auto getLights() const noexcept -> const LightRegistry&;

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
            friend Scene;

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

    private:
        Node root;

        LightRegistry lightRegistry;
        std::unordered_map<Light, ShadowNode> shadowNodes;
    };
} // namespace trc
