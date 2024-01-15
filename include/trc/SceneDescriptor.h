#pragma once

#include <cstddef>

#include "trc/Types.h"
#include "trc/base/Buffer.h"
#include "trc/core/DescriptorProvider.h"

namespace trc
{
    class Instance;
    class RasterSceneModule;
    class RaySceneModule;
    class SceneBase;

    /**
     * binding 0: Lights (uniform buffer)
     * binding 1: Ray drawable data (storage buffer)
     */
    class SceneDescriptor
    {
    public:
        explicit SceneDescriptor(const Instance& instance);

        void update(const SceneBase& scene);

        auto getProvider() const noexcept -> const DescriptorProviderInterface&;

    private:
        class SceneDescriptorProvider : public DescriptorProviderInterface
        {
        public:
            explicit SceneDescriptorProvider(const SceneDescriptor& descriptor);

            auto getDescriptorSetLayout() const noexcept -> vk::DescriptorSetLayout override;
            void bindDescriptorSet(
                vk::CommandBuffer cmdBuf,
                vk::PipelineBindPoint bindPoint,
                vk::PipelineLayout pipelineLayout,
                ui32 setIndex
            ) const override;

        private:
            const SceneDescriptor& descriptor;
        };

        void createDescriptors();
        void writeDescriptors();

        void updateRasterData(const RasterSceneModule& scene);
        void updateRayData(const RaySceneModule& scene);

        const Instance& instance;
        const Device& device;

        vk::UniqueDescriptorSetLayout descLayout;
        vk::UniqueDescriptorPool descPool;
        vk::UniqueDescriptorSet descSet;
        SceneDescriptorProvider provider{ *this };

        Buffer lightBuffer;
        ui8* lightBufferMap;  // Persistent mapping
        Buffer drawableDataBuf;
        std::byte* drawableBufferMap;
    };
} // namespace trc
