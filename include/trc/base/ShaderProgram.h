#pragma once

#include <string>
#include <vector>
#include <optional>
#include <filesystem>

#include "trc/base/Device.h"

namespace trc
{
    namespace fs = std::filesystem;

    class ShaderProgram
    {
    public:
        struct ShaderStageInfo
        {
            ShaderStageInfo(vk::ShaderStageFlagBits type, const std::string& code);
            ShaderStageInfo(vk::ShaderStageFlagBits type,
                            const std::string& code,
                            vk::SpecializationInfo spec);

            vk::ShaderStageFlagBits type;
            const std::string& shaderCode;
            std::optional<vk::SpecializationInfo> specializationInfo{ std::nullopt };
        };

        explicit
        ShaderProgram(const trc::Device& device);
        ShaderProgram(const trc::Device& device,
                      const vk::ArrayProxy<const ShaderStageInfo>& stages);

        void addStage(const ShaderStageInfo& stage);
        void setSpecialization(vk::ShaderStageFlagBits stage, vk::SpecializationInfo info);

        auto getStageCreateInfo() const & -> const std::vector<vk::PipelineShaderStageCreateInfo>&;

    private:
        const Device& device;

        std::vector<vk::UniqueShaderModule> modules;
        std::vector<std::unique_ptr<vk::SpecializationInfo>> specInfos;
        std::vector<vk::PipelineShaderStageCreateInfo> createInfos;
    };

    /**
     * @brief Create a shader module from shader code
     */
    auto makeShaderModule(const trc::Device& device, const std::string& code)
        -> vk::UniqueShaderModule;

    /**
     * @brief Create a shader module from shader code
     */
    auto makeShaderModule(vk::Device device, const std::string& code)
        -> vk::UniqueShaderModule;
} // namespace trc
