#pragma once

#include <generator>

#include "trc/Camera.h"
#include "trc/RenderPassShadow.h"
#include "trc/Types.h"
#include "trc/base/Buffer.h"
#include "trc/core/DescriptorProvider.h"
#include "trc/util/UnorderedCache.h"

namespace trc
{
    class RenderPassShadow;
    class SceneBase;

    /**
     * @brief Creation parameters for ShadowPool
     */
    struct ShadowPoolCreateInfo
    {
        ui32 maxShadowMaps{ 1 };
    };

    /**
     * @brief Creation parameters for shadow maps
     */
    struct ShadowMapCreateInfo
    {
        uvec2 shadowMapResolution;
        s_ptr<Camera> camera;  // Shall not be nullptr
    };

    /**
     * @brief Information about an allocated shadow map
     */
    struct ShadowMap
    {
        ShadowMap(const Device& device, ui32 index, const ShadowMapCreateInfo& info);

        auto getCamera() -> Camera&;
        auto getCamera() const -> const Camera&;

        auto getRenderPass() -> RenderPassShadow&;
        auto getRenderPass() const -> const RenderPassShadow&;

        auto getImage() const -> vk::Image;
        auto getImageView() const -> vk::ImageView;
        auto getShadowMatrixIndex() const -> ui32;

    private:
        ui32 index;
        s_ptr<RenderPassShadow> renderPass;
        s_ptr<Camera> camera;
    };

    /**
     * @brief A pool of shadow map device resources.
     */
    class ShadowPool
    {
    public:
        ShadowPool(const Device& device,
                   const ShadowPoolCreateInfo& info,
                   const DeviceMemoryAllocator& alloc = DefaultDeviceMemoryAllocator{});

        /**
         * @brief Update shadow matrices
         */
        void update();

        /**
         * @param index An index to locate the shadow map on the device. Must
         *              not exceed the pool's maximum number of shadow maps.
         *
         * @throw std::invalid_argument if `index` exceeds the greatest allowed
         *                              index as specified by the pool's maximum
         *                              number of shadow maps.
         */
        auto allocateShadow(ui32 index, const ShadowMapCreateInfo& info) -> s_ptr<ShadowMap>;

        auto getMatrixBuffer() const -> const Buffer&;
        auto getShadows() const -> std::generator<const ShadowMap&>;

    private:
        const Device& device;
        const ui32 kMaxShadowMaps;

        UnorderedCache<ShadowMap> shadowMaps;

        void updateMatrixBuffer();
        Buffer shadowMatrixBuffer;
        mat4* shadowMatrixBufferMap;
    };

    class ShadowDescriptor
    {
    public:
        class DescriptorSet : public DescriptorProviderInterface
        {
        public:
            DescriptorSet(const Device& device,
                          vk::UniqueDescriptorSet descSet,
                          const ShadowPool& pool);

            /**
             * Update content of the shadow matrix buffer.
             * Update descriptor bindings for shadow images.
             */
            void update(const Device& device, const ShadowPool& shadows);

            void bindDescriptorSet(vk::CommandBuffer cmdBuf,
                                   vk::PipelineBindPoint bindPoint,
                                   vk::PipelineLayout pipelineLayout,
                                   ui32 setIndex
                                   ) const override;

        private:
            vk::UniqueDescriptorSet descSet;
        };

        ShadowDescriptor(const Device& device,
                         ui32 maxShadowMaps,
                         ui32 maxDescriptorSets);

        auto getDescriptorSetLayout() const -> vk::DescriptorSetLayout;
        auto makeDescriptorSet(const Device& device, const ShadowPool& shadowPool)
            -> s_ptr<DescriptorSet>;

    private:
        vk::UniqueDescriptorSetLayout descLayout;
        vk::UniqueDescriptorPool descPool;
    };
} // namespace trc
