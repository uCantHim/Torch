#pragma once

#include "core/Window.h"
#include "core/RenderPass.h"
#include "core/Pipeline.h"
#include "GBuffer.h"
#include "RayBuffer.h"

namespace trc {
    class AssetRegistry;
    class RenderTarget;
}

namespace trc::rt
{
    struct FinalCompositingPassCreateInfo
    {
        vkb::FrameSpecific<GBuffer>* gBuffer{ nullptr };
        vkb::FrameSpecific<RayBuffer>* rayBuffer{ nullptr };
    };

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
        FinalCompositingPass(const Instance& instance,
                             const RenderTarget& output,
                             const FinalCompositingPassCreateInfo& info);

        void begin(vk::CommandBuffer cmdBuf, vk::SubpassContents, FrameRenderState&) override;
        void end(vk::CommandBuffer cmdBuf) override;

        /**
         * binding 0 (g-buffer normals):       image2D rgba16
         * binding 1 (g-buffer albedo):        uimage2D r32ui
         * binding 2 (g-buffer materials):     uimage2D r32ui
         * binding 3 (ray-buffer reflections): image2D rgba8
         */
        auto getInputImageDescriptor() const -> const DescriptorProviderInterface&;

    private:
        static constexpr uvec3 COMPUTE_LOCAL_SIZE{ 10, 10, 1 };

        const RenderTarget& renderTarget;
        const uvec3 computeGroupSize;

        vk::UniqueDescriptorPool pool;
        vk::UniqueDescriptorSetLayout inputLayout;
        vk::UniqueDescriptorSetLayout outputLayout;
        vkb::FrameSpecific<vk::UniqueDescriptorSet> inputSets;
        vkb::FrameSpecific<vk::UniqueDescriptorSet> outputSets;

        std::vector<vk::UniqueSampler> depthSamplers;

        FrameSpecificDescriptorProvider inputSetProvider;
        FrameSpecificDescriptorProvider outputSetProvider;

        PipelineLayout computePipelineLayout;
        Pipeline computePipeline;
    };
} // namespace trc::rt
