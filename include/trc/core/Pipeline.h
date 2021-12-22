#pragma once

#include <variant>

#include <vkb/Device.h>

#include "../Types.h"
#include "PipelineLayout.h"

namespace trc
{
    /**
     * @brief Helper to create a pipeline layout
     *
     * @param descriptorSetLayouts List of descriptor set layouts in the
     *                             pipeline
     * @param pushConstantRanges   List of push constant ranges in the
     *                             pipeline
     *
     * @return vk::UniquePipeline
     */
    inline auto makePipelineLayout(
        const vkb::Device& device,
        const std::vector<vk::DescriptorSetLayout>& descriptorSetLayouts,
        const std::vector<vk::PushConstantRange>& pushConstantRanges)
    {
        return PipelineLayout(device, descriptorSetLayouts, pushConstantRanges);
    }

    /**
     * @brief Base class for all pipelines
     */
    class Pipeline
    {
    public:
        using ID = TypesafeID<Pipeline, ui32>;
        using UniquePipelineHandleType = std::variant<
            vk::UniqueHandle<vk::Pipeline, vk::DispatchLoaderStatic>,  // vk::UniquePipeline
            vk::UniqueHandle<vk::Pipeline, vk::DispatchLoaderDynamic>
        >;

        Pipeline(const Pipeline&) = delete;
        Pipeline& operator=(const Pipeline&) = delete;

        Pipeline(PipelineLayout& layout,
                 vk::UniquePipeline pipeline,
                 vk::PipelineBindPoint bindPoint);
        Pipeline(PipelineLayout& layout,
                 UniquePipelineHandleType pipeline,
                 vk::PipelineBindPoint bindPoint);
        Pipeline(Pipeline&&) noexcept = default;
        ~Pipeline() = default;

        Pipeline& operator=(Pipeline&&) noexcept = default;

        auto operator*() const noexcept -> vk::Pipeline;
        auto get() const noexcept -> vk::Pipeline;

        /**
         * @brief Bind the pipeline and all static descriptor sets
         *
         * @param vk::CommandBuffer cmdBuf The command buffer to record the
         *                                 pipeline bind to.
         */
        void bind(vk::CommandBuffer cmdBuf) const;

        auto getLayout() noexcept -> PipelineLayout&;
        auto getLayout() const noexcept -> const PipelineLayout&;

    private:
        PipelineLayout* layout;
        UniquePipelineHandleType pipelineStorage;
        vk::Pipeline pipeline;
        vk::PipelineBindPoint bindPoint;
    };



    /**
     * @brief Create a compute shader pipeline
     */
    auto makeComputePipeline(const vkb::Device& device,
                             PipelineLayout& layout,
                             vk::UniqueShaderModule shader,
                             vk::PipelineCreateFlags flags = {},
                             const std::string& entryPoint = "main") -> Pipeline;
} // namespace trc
