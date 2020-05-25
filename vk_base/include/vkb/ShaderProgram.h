#pragma once

#include <vector>
#include <string>

#include "VulkanBase.h"

namespace vkb
{

class ShaderProgram : public VulkanBase
{
public:
    using ShaderStages = std::vector<vk::PipelineShaderStageCreateInfo>;

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

    [[nodiscard]]
    auto getStages() const noexcept -> const ShaderStages&;

private:
    ShaderStages stages;
    std::vector<vk::UniqueShaderModule> modules;
};


auto readFile(const std::string& path) -> std::vector<char>;
auto createShaderModule(std::vector<char> code) -> vk::UniqueShaderModule;


} // namespace vkb
