#pragma once

#include <vkb/Buffer.h>

#include "Light.h"
#include "Node.h"
#include "RenderPassShadow.h"

namespace trc
{
    class Instance;

    /**
     * @brief Collection and management unit for lights and shadows
     */
    class LightRegistry
    {
    public:
        /**
         * TODO: Rework some of the shadow stuff
         */
        friend class ShadowDescriptor;



        static constexpr ui32 DEFAULT_MAX_LIGHTS = 32;
        static constexpr ui32 MAX_SHADOW_MAPS = 256;

        explicit LightRegistry(const Instance& instance, ui32 maxLights = DEFAULT_MAX_LIGHTS);

        /**
         * @brief Update lights in the registry
         *
         * Updates the light buffer. Applies transformations of attached
         * light nodes to their lights.
         */
        void update();

        ui32 getMaxLights() const noexcept;

        /**
         * @return const Light& The added light
         */
        auto addLight(Light light) -> Light&;

        /**
         * Also removes a light node that has the light attached if such
         * a node exists.
         */
        void removeLight(const Light& light);

        /**
         * @brief Storage of and handle to a light's shadow
         *
         * A shadow consists of one (e.g. sun lights) or more (e.g. point
         * lights) cameras that define the shadow direction and projection.
         * The handle gives limited access to these cameras.
         */
        struct ShadowInfo
        {
            // TODO: Remove this shit
            friend class ShadowDescriptor;


            /**
             * @return Node& A node that all shadow cameras are attached to
             */
            auto getNode() noexcept -> Node&;

            /**
             * @brief Set a projection matrix on all shadow cameras
             */
            void setProjectionMatrix(mat4 proj) noexcept;

        private:
            friend LightRegistry;

            std::vector<Camera> shadowCameras;
            Node parentNode;
        };

        /**
         * @brief Enable shadows for a specific light
         *
         * In order to work properly, a position should be set on sun
         * lights before passing them to this function.
         *
         * @param Light& light      The light that shall cast shadows.
         * @param uvec2  resolution The resolution of the created shadow
         *                          map. This can not be changed later on.
         * @param ShadowStage& renderStage The stage that created render
         *                                 passes shall be attached to.
         *
         * @return ShadowInfo&
         *
         * @throw std::invalid_argument if shadows are already enabled on the light
         * @throw std::runtime_error if something unexpected happens
         */
        auto enableShadow(Light& light, uvec2 shadowResolution) -> ShadowInfo&;

        /**
         * Does nothing if shadows are not enabled for the light
         */
        void disableShadow(Light& light);

        auto getLightBuffer() const noexcept -> vk::Buffer;
        auto getShadowMatrixBuffer() const noexcept -> vk::Buffer;

    private:
        const ui32 maxLights;
        const ui32 maxShadowMaps;

        /**
         * I could keep all lights in one array instead and sort that array
         * by light type before every buffer update. But I wanted to try
         * something different and see how it works out.
         */
        bool lightExists(const Light& light);
        std::vector<u_ptr<Light>> sunLights;
        std::vector<u_ptr<Light>> pointLights;
        std::vector<u_ptr<Light>> ambientLights;

        // Must be done every frame in case light properties change
        void updateLightBuffer();
        vkb::Buffer lightBuffer;

        /**
         * This must be called every frame in case a shadow matrix changes
         */
        void updateShadowMatrixBuffer();
        std::unordered_map<Light*, ShadowInfo> shadows;
        vkb::Buffer shadowMatrixBuffer;
    };
} // namespace trc
