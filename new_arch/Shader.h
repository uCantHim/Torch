#pragma once

#include <filesystem>
namespace fs = std::filesystem;

#include <vulkan/vulkan.hpp>

class ShaderModule
{
public:
    explicit ShaderModule(const fs::path& filePath);

    auto makePipelineShaderStage(const std::string& entryPoint) const noexcept
        -> vk::PipelineShaderStageCreateInfo;
};
