/**
 * Torch-specific utilities to handle shader resources at runtime.
 */

#pragma once

#include <cassert>

#include <limits>
#include <vector>

#include "ShaderResourceInterface.h"
#include "trc/drawable/Drawable.h"

namespace trc
{
    struct MaterialRuntimeConfig;

    /**
     * @brief Calculates push constant offsets
     *
     * Uses a user-defined configuration to translate virtual push constant
     * identifiers to a push constant's offset.
     */
    class RuntimePushConstantHandler
    {
    public:
        RuntimePushConstantHandler(const ShaderResources& resources,
                                   const MaterialRuntimeConfig& conf);

        template<typename T>
        void pushConstants(vk::CommandBuffer cmdBuf,
                           vk::PipelineLayout layout,
                           ui32 pushConstantId,
                           T&& value) const;

    private:
        std::vector<ui32> offsets;
    };



    template<typename T>
    void RuntimePushConstantHandler::pushConstants(
        vk::CommandBuffer cmdBuf,
        vk::PipelineLayout layout,
        ui32 pushConstantId,
        T&& value) const
    {
        assert(offsets.size() > pushConstantId);
        assert(offsets[pushConstantId] != std::numeric_limits<ui32>::max());

        cmdBuf.pushConstants<T>(layout, vk::ShaderStageFlagBits::eVertex,
                                offsets[pushConstantId], value);
    }
}
