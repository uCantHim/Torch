#pragma once

#include <string>
#include <unordered_map>

#include "MaterialCompiler.h"
#include "MaterialOutputNode.h"
#include "RuntimeResourceHandler.h"
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
        MaterialOutputNode::ParameterID colorParam;
        MaterialOutputNode::ParameterID normalParam;
        MaterialOutputNode::ParameterID roughnessParam;
        MaterialOutputNode::ParameterID emissiveParam;
    };

    struct MaterialRuntimeConfig
    {
        struct DescriptorInfo
        {
            auto operator<=>(const DescriptorInfo&) const = default;

            ui32 index;
            bool isStatic;
        };

        std::unordered_map<std::string, DescriptorInfo> descriptorInfos;
        std::vector<std::pair<ui32, ResourceID>> pushConstantIds;
    };

    auto mergeRuntimeConfigs(const MaterialRuntimeConfig& a, const MaterialRuntimeConfig& b)
        -> MaterialRuntimeConfig;

    struct MaterialRuntimeInfo
    {
        MaterialRuntimeInfo(
            const MaterialRuntimeConfig& runtimeConf,
            PipelineVertexParams vert,
            PipelineFragmentParams frag,
            std::unordered_map<vk::ShaderStageFlagBits, MaterialCompileResult> stages
        );

        auto getShaderGlslCode(vk::ShaderStageFlagBits stage) const -> const std::string&;

        auto makePipeline(AssetManager& assetManager) -> Pipeline::ID;

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
                               const MaterialRuntimeConfig& descConf) -> PipelineLayoutTemplate;

        PipelineVertexParams vertParams;
        PipelineFragmentParams fragParams;

        PipelineLayoutTemplate layoutTemplate;
        std::unordered_map<vk::ShaderStageFlagBits, ShaderStageInfo> shaderStages;

        std::vector<TextureHandle> textureHandles;
        RuntimePushConstantHandler vertResourceHandler;
    };
} // namespace trc
