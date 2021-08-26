#pragma once

#include <vector>
#include <unordered_map>

#include "Types.h"
#include "core/SceneBase.h"
#include "LightRegistry.h"
#include "ShadowPool.h"
#include "Node.h"

namespace trc
{
    class Scene : public SceneBase
    {
    public:
        auto getRoot() noexcept -> Node&;
        auto getRoot() const noexcept -> const Node&;

        /**
         * @brief Traverse the node tree and update the transform of each node
         *
         * This it NOT called automatically. You don't have to ever call
         * this if you don't want to, though you should if you want to
         * attach nodes to the scene. Call this once per frame.
         *
         * You may also call this from another thread than the rendering
         * thread, though you have to take care because adding nodes while
         * traversing the tree is NOT safe and will probably crash the
         * program.
         */
        void updateTransforms();

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
         * In order to work properly, a position should be set on sun
         * lights before passing them to this function.
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
