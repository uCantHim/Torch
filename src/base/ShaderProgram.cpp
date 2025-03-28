#include "trc/base/ShaderProgram.h"

#include <fstream>



auto trc::readSpirvFile(const fs::path& path) -> std::vector<uint32_t>
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        throw std::runtime_error("[In readSpirvFile]: Unable to open file " + path.string());
    }

    const auto fileSize = file.tellg();
    if (fileSize % sizeof(uint32_t) != 0)
    {
        throw std::runtime_error("[In readSpirvFile]: File size is not a multiple of 4: file is"
                                 " not a valid SPIRV file!");
    }

    std::vector<uint32_t> result(fileSize / sizeof(uint32_t));
    file.seekg(0);
    file.read(reinterpret_cast<char*>(result.data()), fileSize);

    return result;
}

auto trc::makeShaderModule(const trc::Device& device, const std::vector<uint32_t>& code)
    -> vk::UniqueShaderModule
{
    assert(!code.empty());

    vk::ShaderModuleCreateInfo info({}, code.size() * sizeof(uint32_t), code.data());
    return device->createShaderModuleUnique(info);
}



trc::ShaderProgram::ShaderProgram(const trc::Device& device)
    :
    device(device)
{
}

void trc::ShaderProgram::addStage(vk::ShaderStageFlagBits type, std::vector<uint32_t> shaderCode)
{
    const auto& code = shaderCodes.emplace_back(std::move(shaderCode));
    const auto& mod = moduleCreateInfos.emplace_back(std::make_unique<vk::ShaderModuleCreateInfo>(
        vk::ShaderModuleCreateInfo{ {}, code.size() * sizeof(uint32_t), code.data() }
    ));

    createInfos.emplace_back(vk::PipelineShaderStageCreateInfo({}, type, VK_NULL_HANDLE, "main", nullptr));
    createInfos.back().setPNext(mod.get());
}

void trc::ShaderProgram::addStage(
    vk::ShaderStageFlagBits type,
    std::vector<uint32_t> shaderCode,
    vk::SpecializationInfo specializationInfo)
{
    addStage(type, std::move(shaderCode));

    // Set the specialization info on the newly created stage
    auto& specInfo = specInfos.emplace_back(
        std::make_unique<vk::SpecializationInfo>(specializationInfo)
    );
    createInfos.back().setPSpecializationInfo(specInfo.get());
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
