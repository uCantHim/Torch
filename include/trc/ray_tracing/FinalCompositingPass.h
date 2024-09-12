#pragma once

#include "trc/core/Pipeline.h"
#include "trc/core/PipelineLayout.h"
#include "trc/core/RenderPipelineTasks.h"
#include "trc/core/RenderTarget.h"
#include "trc/ray_tracing/RayBuffer.h"

namespace trc::rt
{
    class CompositingDescriptor
    {
    public:
        CompositingDescriptor(const Device& device, ui32 maxDescriptorSets);

        auto makeDescriptorSet(const Device& device,
                               const RayBuffer& rayBuffer,
                               vk::ImageView outputImage)
            -> vk::UniqueDescriptorSet;

        auto getDescriptorSetLayout() const -> vk::DescriptorSetLayout;

    private:
        vk::UniqueDescriptorPool pool;

        /**
         * binding 0 (ray-buffer reflections): image2D rgba8
         * binding 1 (destination image):      image2D rgba8
         */
        vk::UniqueDescriptorSetLayout layout;
    };

    /**
     * @brief Compute pass that merges raster and rt results together
     */
    class FinalCompositingDispatcher
    {
    public:
        FinalCompositingDispatcher(const Device& device,
                             const RayBuffer& rayBuffer,
                             const Viewport& renderTarget,
                             CompositingDescriptor& descriptor);

        void createTasks(ViewportDrawTaskQueue& taskQueue);

    private:
        static constexpr uvec3 COMPUTE_LOCAL_SIZE{ 10, 10, 1 };

        const Device& device;
        const uvec3 computeGroupSize;

        vk::UniqueDescriptorSet descSet;
        PipelineLayout computePipelineLayout;
        Pipeline computePipeline;

        s_ptr<DescriptorProvider> descriptorProvider;
        vk::Image targetImage;
    };
} // namespace trc::rt
