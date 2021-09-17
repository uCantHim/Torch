#pragma once

#include "core/Window.h"
#include "core/RenderPass.h"
#include "core/Pipeline.h"
#include "GBuffer.h"
#include "RayBuffer.h"

namespace trc::rt
{
    struct FinalCompositingPassCreateInfo
    {
        vkb::FrameSpecific<GBuffer>* gBuffer;
        vkb::FrameSpecific<RayBuffer>* rayBuffer;
    };

    auto getFinalCompositingStage() -> RenderStageType::ID;

    /**
     * @brief Compute pass that merges raster and rt results together
     */
    class FinalCompositingPass : public RenderPass
    {
    public:
        static constexpr ui32 NUM_SUBPASSES{ 1 };

        /**
         * @brief
         */
        FinalCompositingPass(const Window& window, const FinalCompositingPassCreateInfo& info);

        void begin(vk::CommandBuffer cmdBuf, vk::SubpassContents subpass) override;
        void end(vk::CommandBuffer cmdBuf) override;

    private:
        static constexpr uvec3 COMPUTE_LOCAL_SIZE{ 10, 10, 1 };

        const vkb::Swapchain& swapchain;
        const uvec3 computeGroupSize;

        vk::UniqueDescriptorPool pool;
        vk::UniqueDescriptorSetLayout inputLayout;
        vk::UniqueDescriptorSetLayout outputLayout;
        vkb::FrameSpecific<vk::UniqueDescriptorSet> inputSets;
        vkb::FrameSpecific<vk::UniqueDescriptorSet> outputSets;

        FrameSpecificDescriptorProvider inputSetProvider;
        FrameSpecificDescriptorProvider outputSetProvider;

        Pipeline computePipeline;
    };
} // namespace trc::rt
