#pragma once

#include <vector>

#include <vkb/Buffer.h>
#include <vkb/Image.h>
#include <vkb/FrameSpecificObject.h>
#include "trc_util/data/ObjectId.h"

#include "Types.h"
#include "core/Camera.h"
#include "core/DescriptorProvider.h"
#include "core/Instance.h"
#include "Framebuffer.h"
#include "RenderPassShadow.h"

namespace trc
{
    class RenderPassShadow;

    /**
     * @brief Information about an allocated shadow map
     */
    struct ShadowMap
    {
        ui32 index;
        RenderPassShadow* renderPass;
        Camera* camera;
    };

    /**
     * @brief Construction parameters for ShadowPool
     */
    struct ShadowPoolCreateInfo
    {
        ui32 maxShadowMaps{ 1 };
    };

    /**
     * @brief Allocation parameters for shadow maps
     */
    struct ShadowCreateInfo
    {
        uvec2 shadowMapResolution;
    };

    /**
     * @brief
     */
    class ShadowPool
    {
    public:
        /**
         * @brief
         */
        ShadowPool(const Window& window, ShadowPoolCreateInfo info);

        /**
         * @brief Update shadow matrices
         *
         * This is only necessary if a shadow's camera's view or projection
         * matrices have changed.
         */
        void update();

        auto allocateShadow(const ShadowCreateInfo& info) -> ShadowMap;
        void freeShadow(const ShadowMap& info);

        auto getProvider() const -> const DescriptorProviderInterface&;

    private:
        const Window& window;
        const vkb::Device& device;
        const vkb::Swapchain& swapchain;

        struct Shadow
        {
            Shadow(const Window& window, ui32 index, uvec2 size);

            ui32 index;

            Camera camera;
            RenderPassShadow renderPass;
        };

        data::IdPool shadowIdPool;
        std::vector<u_ptr<Shadow>> shadows;

        void updateMatrixBuffer();
        vkb::Buffer shadowMatrixBuffer;
        mat4* shadowMatrixBufferMap;

        //////////////
        // Descriptors
        void createDescriptors(ui32 maxShadowMaps);
        void writeDescriptors(ui32 frameIndex);

        vk::UniqueDescriptorSetLayout descLayout;
        vk::UniqueDescriptorPool descPool;
        vkb::FrameSpecific<vk::UniqueDescriptorSet> descSets;
        FrameSpecificDescriptorProvider provider;
    };
} // namespace trc
