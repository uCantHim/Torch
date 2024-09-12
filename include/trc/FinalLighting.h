#pragma once

#include "trc/Types.h"
#include "trc/core/Pipeline.h"
#include "trc/core/RenderPipelineTasks.h"

namespace trc
{
    class Pipeline;
    class PipelineLayout;

    /**
     * @brief Concrete resources to draw to a single image
     *
     * Create this via `FinalLighting::makeDrawConfig`.
     */
    class FinalLightingDispatcher
    {
    public:
        FinalLightingDispatcher(const Viewport& viewport,
                                Pipeline::ID computePipeline,
                                vk::UniqueDescriptorSet renderTargetDescSet);

        /**
         * @brief Spawn a compute task in the `stages::finalLighting` stage
         */
        void createTasks(ViewportDrawTaskQueue& queue);

    private:
        static constexpr uvec3 kLocalGroupSize{ 16, 16, 1 };

        const uvec3 groupCount;
        const ivec2 renderOffset;
        const uvec2 renderSize;
        const vk::Image targetImage;

        Pipeline::ID pipeline;
        vk::UniqueDescriptorSet descSet;
    };

    /**
     * @brief General resources for the final lighting stage
     *
     * Factory for per-viewport configurations.
     */
    class FinalLighting
    {
    public:
        static constexpr auto OUTPUT_IMAGE_DESCRIPTOR{ "final_lighting_output_image" };

        FinalLighting(const Device& device, ui32 maxInstances);

        /**
         * @brief Create resources that can execute the lighting stage on an
         *        image.
         */
        auto makeDrawConfig(const Device& device, const Viewport& viewport)
            -> u_ptr<FinalLightingDispatcher>;

        auto getDescriptorSetLayout() const -> vk::DescriptorSetLayout;

    private:
        vk::UniqueDescriptorPool descPool;
        vk::UniqueDescriptorSetLayout descLayout;

        PipelineLayout::ID layout;
        Pipeline::ID pipeline;
    };
} // namespace trc
