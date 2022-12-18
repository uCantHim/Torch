#include "trc/base/ShaderProgram.h"

#include <fstream>
#include <sstream>



trc::ShaderProgram::ShaderStageInfo::ShaderStageInfo(
    vk::ShaderStageFlagBits type,
    const std::string& code)
    :
    type(type),
    shaderCode(code),
    specializationInfo(std::nullopt)
{
}

trc::ShaderProgram::ShaderStageInfo::ShaderStageInfo(
    vk::ShaderStageFlagBits type,
    const std::string& code,
    vk::SpecializationInfo spec)
    :
    type(type),
    shaderCode(code),
    specializationInfo(spec)
{
}



trc::ShaderProgram::ShaderProgram(const trc::Device& device)
    :
    device(device)
{
}

trc::ShaderProgram::ShaderProgram(
    const trc::Device& device,
    const vk::ArrayProxy<const ShaderStageInfo>& stages)
    :
    device(device)
{
    for (const auto& stage : stages)
    {
        addStage(stage);
    }
}

void trc::ShaderProgram::addStage(const ShaderStageInfo& stage)
{
    auto& mod = modules.emplace_back(makeShaderModule(device, stage.shaderCode));

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

void trc::ShaderProgram::setSpecialization(
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

auto trc::ShaderProgram::getStageCreateInfo() const &
    -> const std::vector<vk::PipelineShaderStageCreateInfo>&
{
    return createInfos;
}



auto trc::makeShaderModule(const trc::Device& device, const std::string& code)
    -> vk::UniqueShaderModule
{
    return makeShaderModule(*device, code);
}

auto trc::makeShaderModule(vk::Device device, const std::string& code)
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
