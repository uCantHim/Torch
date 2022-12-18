#include "trc/base/ShaderProgram.h"

#include <fstream>
#include <sstream>



auto trc::makeShaderModule(const trc::Device& device, const std::string& code)
    -> vk::UniqueShaderModule
{
    assert(!code.empty());

    vk::ShaderModuleCreateInfo info(
        vk::ShaderModuleCreateFlags(),
        code.size(),
        reinterpret_cast<const uint32_t*>(code.data())
    );

    return device->createShaderModuleUnique(info);
}



trc::ShaderProgram::ShaderProgram(const trc::Device& device)
    :
    device(device)
{
}

void trc::ShaderProgram::addStage(vk::ShaderStageFlagBits type, const std::string& shaderCode)
{
    auto& mod = modules.emplace_back(makeShaderModule(device, shaderCode));
    createInfos.emplace_back(vk::PipelineShaderStageCreateInfo({}, type, *mod, "main", nullptr));
}

void trc::ShaderProgram::addStage(
    vk::ShaderStageFlagBits type,
    const std::string& shaderCode,
    vk::SpecializationInfo specializationInfo)
{
    addStage(type, shaderCode);

    // Set the specialization info on the newly created stage
    vk::SpecializationInfo* specInfo = specInfos.emplace_back(
        new vk::SpecializationInfo(specializationInfo)
    ).get();
    createInfos.back().setPSpecializationInfo(specInfo);
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
