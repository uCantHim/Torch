#pragma once

#include <cstddef>

#include "trc/Types.h"
#include "trc/base/Buffer.h"
#include "trc/core/DescriptorProvider.h"

namespace trc
{
    class LightSceneModule;
    class RaySceneModule;
    class SceneBase;

    /**
     * binding 0: Lights (uniform buffer)
     * binding 1: Ray drawable data (storage buffer)
     */
    class SceneDescriptor : public DescriptorProviderInterface
    {
    public:
        explicit SceneDescriptor(const Device& device);

        void update(const SceneBase& scene);

        auto getDescriptorSetLayout() const noexcept -> vk::DescriptorSetLayout;
        void bindDescriptorSet(vk::CommandBuffer cmdBuf,
                               vk::PipelineBindPoint bindPoint,
                               vk::PipelineLayout pipelineLayout,
                               ui32 setIndex
                               ) const override;

    private:
        void createDescriptors();
        void writeDescriptors();

        void updateLightData(const LightSceneModule& scene);
        void updateRayData(const RaySceneModule& scene);

        const Device& device;

        vk::UniqueDescriptorSetLayout descLayout;
        vk::UniqueDescriptorPool descPool;
        vk::UniqueDescriptorSet descSet;

        Buffer lightBuffer;
        ui8* lightBufferMap;  // Persistent mapping
        Buffer drawableDataBuf;
        std::byte* drawableBufferMap;
    };
} // namespace trc
