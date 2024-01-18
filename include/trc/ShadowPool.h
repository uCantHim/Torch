#pragma once

#include <vector>

#include <componentlib/Table.h>

#include "trc_util/data/IdPool.h"

#include "trc/Camera.h"
#include "trc/RenderPassShadow.h"
#include "trc/Types.h"
#include "trc/base/Buffer.h"
#include "trc/base/FrameSpecificObject.h"
#include "trc/core/DescriptorProvider.h"
#include "trc/core/Instance.h"

namespace trc
{
    class RenderPassShadow;

    /**
     * @brief Information about an allocated shadow map
     */
    struct ShadowMap
    {
        ui32 index;
        s_ptr<RenderPassShadow> renderPass;
        s_ptr<Camera> camera;
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
    class ShadowPool : public DescriptorProviderInterface
    {
    public:
        /**
         * @brief
         */
        ShadowPool(const Device& device, const FrameClock& clock, ShadowPoolCreateInfo info);

        /**
         * @brief Update shadow matrices
         *
         * This is only necessary if a shadow's camera's view or projection
         * matrices have changed.
         */
        void update();

        auto allocateShadow(const ShadowCreateInfo& info) -> ShadowMap;
        void freeShadow(const ShadowMap& info);

        auto getDescriptorSetLayout() const -> vk::DescriptorSetLayout;
        void bindDescriptorSet(
            vk::CommandBuffer cmdBuf,
            vk::PipelineBindPoint bindPoint,
            vk::PipelineLayout pipelineLayout,
            ui32 setIndex
        ) const override;

    private:
        const Device& device;
        const FrameClock& clock;

        struct Shadow
        {
            Shadow(const Device& device, const FrameClock& clock, ui32 index, uvec2 size);

            ui32 index;

            s_ptr<Camera> camera;
            s_ptr<RenderPassShadow> renderPass;
        };

        const ui32 kMaxShadowMaps;
        data::IdPool<ui32> shadowIdPool;
        componentlib::Table<u_ptr<Shadow>> shadows;

        void updateMatrixBuffer();
        Buffer shadowMatrixBuffer;
        mat4* shadowMatrixBufferMap;

        //////////////
        // Descriptors
        void createDescriptors(ui32 maxShadowMaps);
        void writeDescriptors(ui32 frameIndex);

        vk::UniqueDescriptorSetLayout descLayout;
        vk::UniqueDescriptorPool descPool;
        FrameSpecific<vk::UniqueDescriptorSet> descSets;
    };
} // namespace trc
