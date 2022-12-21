#pragma once

#include <vector>

#include "ShaderCapabilityConfig.h"
#include "ShaderResourceInterface.h"
#include "trc/assets/Texture.h"
#include "trc/core/Pipeline.h"

namespace trc
{
    class AssetManager;

    using ResourceID = ShaderCapabilityConfig::ResourceID;

    struct MaterialRuntime
    {
        MaterialRuntime(Pipeline::ID pipeline,
                        std::vector<ShaderResources::PushConstantInfo> pushConstantConfig,
                        std::vector<TextureHandle> loadedTextures);

        auto getPipeline() const -> Pipeline::ID;

        template<typename T>
        void pushConstants(vk::CommandBuffer cmdBuf,
                           vk::PipelineLayout layout,
                           ui32 pushConstantId,
                           T&& value) const;

    private:
        Pipeline::ID pipeline;
        std::vector<TextureHandle> loadedTextures;

        std::vector<ui32> pcOffsets;
    };



    template<typename T>
    void MaterialRuntime::pushConstants(
        vk::CommandBuffer cmdBuf,
        vk::PipelineLayout layout,
        ui32 pushConstantId,
        T&& value) const
    {
        assert(pcOffsets.size() > pushConstantId);
        assert(pcOffsets[pushConstantId] != std::numeric_limits<ui32>::max());
        assert(pushConstantId == pcOffsets.size() - 1
               || (pcOffsets[pushConstantId + 1] + pcOffsets[pushConstantId]) == sizeof(T));

        cmdBuf.pushConstants<T>(layout, vk::ShaderStageFlagBits::eVertex,
                                pcOffsets[pushConstantId], value);
    }
} // namespace trc
