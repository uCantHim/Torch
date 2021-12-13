#include "ShaderProgram.h"

#include <fstream>
#include <sstream>



vkb::ShaderProgram::ShaderStageInfo::ShaderStageInfo(
    vk::ShaderStageFlagBits type,
    const std::string& code)
    :
    type(type),
    shaderCode(code),
    specializationInfo(std::nullopt)
{
}

vkb::ShaderProgram::ShaderStageInfo::ShaderStageInfo(
    vk::ShaderStageFlagBits type,
    const std::string& code,
    vk::SpecializationInfo spec)
    :
    type(type),
    shaderCode(code),
    specializationInfo(spec)
{
}



vkb::ShaderProgram::ShaderProgram(const vkb::Device& device)
    :
    device(device)
{
}

vkb::ShaderProgram::ShaderProgram(
    const vkb::Device& device,
    const vk::ArrayProxy<const ShaderStageInfo>& stages)
    :
    device(device)
{
    for (const auto& stage : stages)
    {
        addStage(stage);
    }
}

void vkb::ShaderProgram::addStage(const ShaderStageInfo& stage)
{
    auto& mod = modules.emplace_back(createShaderModule(device, stage.shaderCode));

    vk::SpecializationInfo* specInfo{ nullptr };
    if (stage.specializationInfo.has_value())
    {
        specInfo = specInfos.emplace_back(
            new vk::SpecializationInfo(stage.specializationInfo.value())
        ).get();
    }

    createInfos.emplace_back(vk::PipelineShaderStageCreateInfo(
        {},
        stage.type,
        *mod,
        "main",
        specInfo
    ));
}

void vkb::ShaderProgram::setSpecialization(
    vk::ShaderStageFlagBits stageType,
    vk::SpecializationInfo info)
{
    for (auto& stage : createInfos)
    {
        if (stage.stage == stageType)
        {
            stage.setPSpecializationInfo(
                specInfos.emplace_back(new vk::SpecializationInfo(info)).get()
            );
        }
    }
}

auto vkb::ShaderProgram::getStageCreateInfo() const &
    -> const std::vector<vk::PipelineShaderStageCreateInfo>&
{
    return createInfos;
}



auto vkb::readFile(const fs::path& path) -> std::string
{
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("[In readFile]: Unable to open file " + path.string());
    }

    std::stringstream buf;
    buf << file.rdbuf();

    return buf.str();
}

auto vkb::createShaderModule(const vkb::Device& device, const std::string& code)
    -> vk::UniqueShaderModule
{
    return createShaderModule(*device, code);
}

auto vkb::createShaderModule(vk::Device device, const std::string& code)
    -> vk::UniqueShaderModule
{
    assert(!code.empty());

    vk::ShaderModuleCreateInfo info(
        vk::ShaderModuleCreateFlags(),
        code.size(),
        reinterpret_cast<const uint32_t*>(code.data())
    );

    return device.createShaderModuleUnique(info);
}
