#pragma once

#include <limits>
#include <vector>

#include "trc/Types.h"
#include "trc/VulkanInclude.h"
#include "trc/core/Pipeline.h"

namespace trc
{
    struct MaterialRuntime
    {
        MaterialRuntime(Pipeline::ID pipeline, s_ptr<std::vector<ui32>> pcOffsets);

        auto getPipeline() const -> Pipeline::ID;

        template<typename T>
        void pushConstants(vk::CommandBuffer cmdBuf,
                           vk::PipelineLayout layout,
                           ui32 pushConstantId,
                           T&& value) const;

    private:
        Pipeline::ID pipeline;
        s_ptr<std::vector<ui32>> pcOffsets;
    };



    template<typename T>
    void MaterialRuntime::pushConstants(
        vk::CommandBuffer cmdBuf,
        vk::PipelineLayout layout,
        ui32 pushConstantId,
        T&& value) const
    {
        assert(pcOffsets->size() > pushConstantId);
        assert(pcOffsets->at(pushConstantId) != std::numeric_limits<ui32>::max());
        assert(pushConstantId < pcOffsets->size());

        cmdBuf.pushConstants<T>(layout, vk::ShaderStageFlagBits::eVertex,
                                pcOffsets->at(pushConstantId), value);
    }
} // namespace trc
