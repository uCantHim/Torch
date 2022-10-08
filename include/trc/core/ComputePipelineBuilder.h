#pragma once

#include "trc/core/PipelineLayout.h"
#include "trc/core/PipelineRegistry.h"

namespace trc
{
    /**
     * @brief
     */
    class ComputePipelineBuilder
    {
    public:
        using Self = ComputePipelineBuilder;
        using ShaderCode = std::string;

        ComputePipelineBuilder() = default;
        explicit ComputePipelineBuilder(const ComputePipelineTemplate& _template);

        auto setProgram(ShaderCode code) -> Self&;

        template<typename T>
        inline auto setSpecializationConstant(ui32 constantId, T&& data) -> Self&;

        /**
         * @build Finish the build process and create a pipeline template
         */
        auto build() const -> ComputePipelineTemplate;

        auto build(const Device& device, PipelineLayout& layout) -> Pipeline;

        /**
         * @brief Build the pipeline and register it at a pipeline registry
         *
         * @param PipelineLayout::ID layout The pipeline's layout
         */
        auto registerPipeline(PipelineLayout::ID layout) -> Pipeline::ID;

    private:
        ComputePipelineTemplate _template;
    };

    auto buildComputePipeline() -> ComputePipelineBuilder;
    auto buildComputePipeline(const ComputePipelineTemplate& t) -> ComputePipelineBuilder;



    template<typename T>
    inline auto ComputePipelineBuilder::setSpecializationConstant(
        ui32 constantId,
        T&& data) -> Self&
    {
        _template.setSpecializationConstant(constantId, std::forward<T>(data));
        return *this;
    }
} // namespace trc
