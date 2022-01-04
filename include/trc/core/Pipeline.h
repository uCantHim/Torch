#pragma once

#include <variant>

#include <vkb/Device.h>

#include "../Types.h"
#include "PipelineLayout.h"

namespace trc
{
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

        Pipeline() = delete;
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
     *
     * A most basic helper to create a compute pipeline quickly.
     *
     * @note No equivalent function exists for graphics pipelines because
     * their creation is much more complex.
     */
    auto makeComputePipeline(const vkb::Device& device,
                             PipelineLayout& layout,
                             const std::string& code,
                             vk::PipelineCreateFlags flags = {},
                             const std::string& entryPoint = "main") -> Pipeline;
} // namespace trc
