#include "ShaderProgram.h"

#include <fstream>
#include <sstream>



vkb::ShaderProgram::ShaderProgram(
    const vkb::Device& device,
    const std::string& vertPath,
    const std::string& fragPath,
    const std::string& geomPath,
    const std::string& tescPath,
    const std::string& tesePath)
    :
    ShaderProgram(
        createShaderModule(device, readFile(vertPath)),
        createShaderModule(device, readFile(fragPath)),
        geomPath.empty() ? vk::UniqueShaderModule{}
                         : createShaderModule(device, readFile(geomPath)),
        tescPath.empty() ? vk::UniqueShaderModule{}
                         : createShaderModule(device, readFile(tescPath)),
        tesePath.empty() ? vk::UniqueShaderModule{}
                         : createShaderModule(device, readFile(tesePath))
    )
{
}

vkb::ShaderProgram::ShaderProgram(
    vk::UniqueShaderModule vertModule,
    vk::UniqueShaderModule fragModule,
    vk::UniqueShaderModule geomModule,
    vk::UniqueShaderModule tescModule,
    vk::UniqueShaderModule teseModule)
{
    assert(vertModule && fragModule);

    stages.emplace_back(
        vk::PipelineShaderStageCreateFlags(),
        vk::ShaderStageFlagBits::eVertex,
        *vertModule,
        "main"
    );
    stages.emplace_back(
        vk::PipelineShaderStageCreateFlags(),
        vk::ShaderStageFlagBits::eFragment,
        *fragModule,
        "main"
    );
    modules.push_back(std::move(vertModule));
    modules.push_back(std::move(fragModule));

    if (geomModule)
    {
        hasGeom = true;

        stages.emplace_back(
            vk::PipelineShaderStageCreateFlags(),
            vk::ShaderStageFlagBits::eGeometry,
            *geomModule,
            "main"
        );
        modules.push_back(std::move(geomModule));
    }

    if (tescModule && teseModule)
    {
        hasTess = true;

        stages.emplace_back(
            vk::PipelineShaderStageCreateFlags(),
            vk::ShaderStageFlagBits::eTessellationControl,
            *tescModule,
            "main"
        );
        stages.emplace_back(
            vk::PipelineShaderStageCreateFlags(),
            vk::ShaderStageFlagBits::eTessellationEvaluation,
            *teseModule,
            "main"
        );
        modules.push_back(std::move(tescModule));
        modules.push_back(std::move(teseModule));
    }
}

auto vkb::ShaderProgram::getStageCreateInfos() const noexcept -> const ShaderStageCreateInfos&
{
    return stages;
}

void vkb::ShaderProgram::setVertexSpecializationConstants(vk::SpecializationInfo* info)
{
    stages[0].setPSpecializationInfo(info);
}

void vkb::ShaderProgram::setFragmentSpecializationConstants(vk::SpecializationInfo* info)
{
    stages[1].setPSpecializationInfo(info);
}

void vkb::ShaderProgram::setGeometrySpecializationConstants(vk::SpecializationInfo* info)
{
    if (hasGeom) {
        stages[2].setPSpecializationInfo(info);
    }
}

void vkb::ShaderProgram::setTessControlSpecializationConstants(vk::SpecializationInfo* info)
{
    if (hasTess)
    {
        if (hasGeom) {
            stages[3].setPSpecializationInfo(info);
        }
        else {
            stages[2].setPSpecializationInfo(info);
        }
    }
}

void vkb::ShaderProgram::setTessEvalSpecializationConstants(vk::SpecializationInfo* info)
{
    if (hasTess)
    {
        if (hasGeom) {
            stages[4].setPSpecializationInfo(info);
        }
        else {
            stages[3].setPSpecializationInfo(info);
        }
    }
}



auto vkb::readFile(const std::string& path) -> std::string
{
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Unable to open file " + path);
    }

    std::stringstream buf;
    buf << file.rdbuf();

    return buf.str();
}

auto vkb::createShaderModule(const vkb::Device& device, const std::string& code)
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
