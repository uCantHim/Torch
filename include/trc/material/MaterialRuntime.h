#pragma once

#include <string>
#include <unordered_map>

#include "RuntimeResourceHandler.h"
#include "ShaderModuleCompiler.h"
#include "ShaderOutputNode.h"
#include "trc/core/Pipeline.h"
#include "trc/core/PipelineLayoutTemplate.h"

namespace trc
{
    class AssetManager;

    using ResourceID = ShaderCapabilityConfig::ResourceID;

    struct PipelineVertexParams
    {
        bool animated;
    };

    struct PipelineFragmentParams
    {
        bool transparent;
    };

    struct ShaderDescriptorConfig
    {
        struct DescriptorInfo
        {
            auto operator<=>(const DescriptorInfo&) const = default;

            ui32 index;
            bool isStatic;
        };

        std::unordered_map<std::string, DescriptorInfo> descriptorInfos;
    };

    auto mergeDescriptorConfigs(const ShaderDescriptorConfig& a, const ShaderDescriptorConfig& b)
        -> ShaderDescriptorConfig;

    struct MaterialRuntimeInfo
    {
        MaterialRuntimeInfo(
            const ShaderDescriptorConfig& descriptorConf,
            PipelineVertexParams vert,
            PipelineFragmentParams frag,
            std::unordered_map<vk::ShaderStageFlagBits, ShaderModule> stages
        );

        auto getShaderGlslCode(vk::ShaderStageFlagBits stage) const -> const std::string&;

        auto makePipeline(AssetManager& assetManager) -> Pipeline::ID;
        void resolveTextureReferences(AssetManager& assetManager);

        auto getPushConstantHandler() const -> const RuntimePushConstantHandler&;

    private:
        struct ShaderStageInfo
        {
            std::string glslCode;
            std::unordered_map<ui32, TextureReference> textures;
        };

        /**
         * TODO Currently not implemented properly:
         *
         *  - non-static descriptor sets at runtime. Would require a
         *    mechanism like the RuntimePushConstantHandler for descriptor
         *    sets
         *
         *  - push constant ranges for different shader stages
         */
        static auto makeLayout(const std::unordered_set<std::string>& descriptorSets,
                               const std::vector<vk::PushConstantRange>& pushConstants,
                               const ShaderDescriptorConfig& descConf) -> PipelineLayoutTemplate;

        PipelineVertexParams vertParams;
        PipelineFragmentParams fragParams;

        PipelineLayoutTemplate layoutTemplate;
        std::unordered_map<vk::ShaderStageFlagBits, ShaderStageInfo> shaderStages;

        std::vector<TextureHandle> textureHandles;
        RuntimePushConstantHandler vertResourceHandler;
    };
} // namespace trc
