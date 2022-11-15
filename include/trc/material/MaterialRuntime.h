#pragma once

#include <string>
#include <unordered_map>

#include "MaterialCompiler.h"
#include "MaterialOutputNode.h"
#include "trc/core/Pipeline.h"
#include "trc/core/PipelineLayoutTemplate.h"

namespace trc
{
    class AssetManager;

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
    };

    struct DescriptorConfig
    {
        struct DescriptorInfo
        {
            ui32 index;
            bool isStatic;
        };

        std::unordered_map<std::string, DescriptorInfo> descriptorInfos;
    };

    struct MaterialRuntimeInfo
    {
        MaterialRuntimeInfo(
            const DescriptorConfig& descConf,
            PipelineVertexParams vert,
            PipelineFragmentParams frag,
            std::unordered_map<vk::ShaderStageFlagBits, MaterialCompileResult> stages
        );

        auto getShaderGlslCode(vk::ShaderStageFlagBits stage) const -> const std::string&;

        auto makePipeline(AssetManager& assetManager) -> Pipeline::ID;

    private:
        struct ShaderStageInfo
        {
            std::string glslCode;
            std::unordered_map<ui32, TextureReference> textures;
        };

        static auto makeLayout(const std::unordered_set<std::string>& descriptorSets,
                               const DescriptorConfig& descConf) -> PipelineLayoutTemplate;

        PipelineVertexParams vertParams;
        PipelineFragmentParams fragParams;

        PipelineLayoutTemplate layoutTemplate;
        std::unordered_map<vk::ShaderStageFlagBits, ShaderStageInfo> shaderStages;
    };
} // namespace trc
