#pragma once

#include <vector>
#include <string>

#include "basics/Device.h"

namespace vkb
{
    /**
     * @brief Shader program wrapper for easy pipeline creation
     *
     * Can be destroyed after a pipeline is created.
     */
    class ShaderProgram
    {
    public:
        using ShaderStageCreateInfos = std::vector<vk::PipelineShaderStageCreateInfo>;

        ShaderProgram(
            const vkb::Device& device,
            const std::string& vertPath,
            const std::string& fragPath,
            const std::string& geomPath = "",
            const std::string& tescPath = "",
            const std::string& tesePath = ""
        );

        ShaderProgram(
            vk::UniqueShaderModule vertModule,
            vk::UniqueShaderModule fragModule,
            vk::UniqueShaderModule geomModule = {},
            vk::UniqueShaderModule tescModule = {},
            vk::UniqueShaderModule teseModule = {}
        );

        ShaderProgram(const ShaderProgram&) = delete;
        ShaderProgram(ShaderProgram&&) noexcept = default;
        ~ShaderProgram() = default;

        ShaderProgram& operator=(const ShaderProgram&) = delete;
        ShaderProgram& operator=(ShaderProgram&&) noexcept = default;

        auto getStageCreateInfos() const noexcept -> const ShaderStageCreateInfos&;

        void setVertexSpecializationConstants(vk::SpecializationInfo* info);
        void setFragmentSpecializationConstants(vk::SpecializationInfo* info);
        void setGeometrySpecializationConstants(vk::SpecializationInfo* info);
        void setTessControlSpecializationConstants(vk::SpecializationInfo* info);
        void setTessEvalSpecializationConstants(vk::SpecializationInfo* info);

    private:
        ShaderStageCreateInfos stages;
        std::vector<vk::UniqueShaderModule> modules;

        bool hasGeom{ false };
        bool hasTess{ false };
    };

    /**
     * @brief Read the contents of a file to a string
     */
    auto readFile(const std::string& path) -> std::string;

    /**
     * @brief Create a shader module from shader code
     */
    auto createShaderModule(const vkb::Device& device, const std::string& code)
        -> vk::UniqueShaderModule;
} // namespace vkb
