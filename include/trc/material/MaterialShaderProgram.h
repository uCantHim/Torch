#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "MaterialRuntime.h"
#include "ShaderModuleCompiler.h"
#include "ShaderResourceInterface.h"
#include "trc/core/PipelineLayoutTemplate.h"
#include "trc/core/PipelineTemplate.h"

namespace trc
{
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

    /**
     * @brief A complete specialization of a base fragment shader
     */
    class MaterialShaderProgram
    {
    public:
        MaterialShaderProgram(std::unordered_map<vk::ShaderStageFlagBits, ShaderModule> stages,
                              const ShaderDescriptorConfig& descriptorConfig);

        auto getLayout() const -> const PipelineLayoutTemplate&;

        auto makeRuntime(Pipeline::ID basePipeline) -> MaterialRuntime;

    private:
        using ShaderStageMap = std::unordered_map<vk::ShaderStageFlagBits, ShaderModule>;

        /** @throw std::runtime_error if compilation fails */
        static auto compileShader(vk::ShaderStageFlagBits shaderStage, const std::string& glslCode)
            -> std::vector<ui32>;

        /** Compile GLSL source to SPIRV */
        static auto makeProgram(const ShaderStageMap& stages,
                                const PipelineLayoutTemplate& layout)
            -> ProgramDefinitionData;

        /**
         * Collect resources and create a pipeline layout for the shader
         * module.
         *
         * TODO Currently not implemented properly:
         *
         *  - non-static descriptor sets at runtime. Would require a
         *    mechanism like the RuntimePushConstantHandler for descriptor
         *    sets
         *
         *  - push constant ranges for different shader stages
         */
        static auto makeLayout(const ShaderStageMap& stages,
                               const ShaderDescriptorConfig& descConf)
            -> PipelineLayoutTemplate;

        /** The program does not yet contain specialization constants! */
        ProgramDefinitionData program;
        PipelineLayoutTemplate layout;

        std::vector<ShaderResources::PushConstantInfo> pushConstantConfig;
        std::vector<std::pair<ui32, TextureReference>> specializationTextures;
    };
}
