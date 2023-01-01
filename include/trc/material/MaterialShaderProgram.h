#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "MaterialRuntime.h"
#include "ShaderModuleCompiler.h"
#include "ShaderResourceInterface.h"
#include "material_shader_program.pb.h"
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

    struct MaterialProgramData
    {
        struct PushConstantRange
        {
            ui32 offset;
            ui32 size;
            vk::ShaderStageFlags shaderStages;

            ui32 userId;
        };

        std::unordered_map<vk::ShaderStageFlagBits, std::vector<ui32>> spirvCode;

        std::vector<std::pair<ui32, AssetReference<Texture>>> textures;
        std::vector<PushConstantRange> pushConstants;
        std::vector<PipelineLayoutTemplate::Descriptor> descriptorSets;

        auto serialize() const -> serial::ShaderProgram;
        void deserialize(const serial::ShaderProgram& program);
        void serialize(std::ostream& os) const;
        void deserialize(std::istream& is);
    };

    /**
     * @brief Create a shader program from a set of shader modules
     */
    auto makeMaterialProgram(std::unordered_map<vk::ShaderStageFlagBits, ShaderModule>)
        -> MaterialProgramData;

    /**
     * @brief A complete specialization of a base fragment shader
     */
    class MaterialShaderProgram
    {
    public:
        MaterialShaderProgram(const MaterialProgramData& data, Pipeline::ID basePipeline);

        auto getLayout() const -> const PipelineLayoutTemplate&;
        auto makeRuntime() const -> MaterialRuntime;

    private:
        using ShaderStageMap = std::unordered_map<vk::ShaderStageFlagBits, ShaderModule>;

        static auto makeLayout(const MaterialProgramData& data) -> PipelineLayoutTemplate;
        static auto makePipeline(const MaterialProgramData& data) -> Pipeline::ID;

        Pipeline::ID basePipeline;
        PipelineLayoutTemplate layout;

        Pipeline::ID pipeline{ Pipeline::ID::NONE };
        s_ptr<std::vector<ui32>> runtimePcOffsets{ new std::vector<ui32> };
        std::vector<TextureHandle> loadedTextures;
    };
}
