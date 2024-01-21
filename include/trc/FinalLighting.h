#pragma once

#include "trc/Types.h"
#include "trc/VulkanInclude.h"
#include "trc/core/RenderPlugin.h"
#include "trc/core/Task.h"

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
        FinalLightingDispatcher(Viewport viewport,
                                Pipeline::ID computePipeline,
                                vk::UniqueDescriptorSet renderTargetDescSet);

        /**
         * @brief Spawn a compute task in the `finalLightingRenderStage` stage
         */
        void createTasks(TaskQueue& queue);

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
        FinalLighting(const Device& device, ui32 maxViewports);

        /**
         * @brief Create resources that can execute the lighting stage on an
         *        image.
         */
        auto makeDrawConfig(const Device& device, Viewport viewport)
            -> u_ptr<FinalLightingDispatcher>;

    private:
        vk::UniqueDescriptorPool descPool;
        vk::UniqueDescriptorSetLayout descLayout;

        PipelineLayout::ID layout;
        Pipeline::ID pipeline;
    };
} // namespace trc
