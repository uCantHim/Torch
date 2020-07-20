#pragma once

#include <vector>
#include <string>

#include "VulkanBase.h"

namespace vkb
{
    /**
     * @brief Shader program wrapper for easy pipeline creation
     *
     * Can be destroyed after a pipeline is created.
     */
    class ShaderProgram : public VulkanBase
    {
    public:
        using ShaderStageCreateInfos = std::vector<vk::PipelineShaderStageCreateInfo>;

        ShaderProgram(
            const std::string& vertPath,
            const std::string& fragPath,
            const std::string& geomPath = "",
            const std::string& tescPath = "",
            const std::string& tesePath = ""
        );

        ShaderProgram(const ShaderProgram&) = delete;
        ShaderProgram(ShaderProgram&&) noexcept = default;
        ~ShaderProgram() = default;

        ShaderProgram& operator=(const ShaderProgram&) = delete;
        ShaderProgram& operator=(ShaderProgram&&) noexcept = default;

        static ShaderProgram create(
            const std::string& vertPath,
            const std::string& fragPath,
            const std::string& geomPath = "",
            const std::string& tescPath = "",
            const std::string& tesePath = ""
        );

        auto getStageCreateInfos() const noexcept -> const ShaderStageCreateInfos&;

    private:
        ShaderStageCreateInfos stages;
        std::vector<vk::UniqueShaderModule> modules;
    };


    auto readFile(const std::string& path) -> std::vector<char>;
    auto createShaderModule(std::vector<char> code) -> vk::UniqueShaderModule;

} // namespace vkb
