#include "PipelineTemplate.h"

#include <vkb/ShaderProgram.h>

#include "core/Instance.h"



bool trc::SpecializationConstantStorage::empty() const
{
    return data.empty();
}

auto trc::SpecializationConstantStorage::makeSpecializationInfo() const -> vk::SpecializationInfo
{
    assert((entries.empty() && data.empty()) || (!entries.empty() && !data.empty()));

    return {
        static_cast<ui32>(entries.size()), entries.data(),
        static_cast<ui32>(data.size()), data.data()
    };
}



auto trc::ProgramDefinitionData::makeProgram(const vkb::Device& device) const
    -> vkb::ShaderProgram
{
    vkb::ShaderProgram program(device);

    for (const auto& [type, stage] : stages)
    {
        if (stage.specConstants.empty()) {
            program.addStage({ type, stage.code });
        }
        else {
            program.addStage({ type, stage.code, stage.specConstants.makeSpecializationInfo() });
        }
    }

    return program;
}



trc::PipelineTemplate::PipelineTemplate(ProgramDefinitionData program, PipelineDefinitionData pipeline)
    :
    program(std::move(program)),
    data(std::move(pipeline))
{
    compileData();
}

auto trc::PipelineTemplate::getProgramData() const -> const ProgramDefinitionData&
{
    return program;
}

auto trc::PipelineTemplate::getPipelineData() const -> const PipelineDefinitionData&
{
    return data;
}

void trc::PipelineTemplate::compileData()
{
    data.viewport = vk::PipelineViewportStateCreateInfo({}, data.viewports, data.scissorRects);

    if (data.viewport.viewportCount == 0)
    {
        const auto& vp = data.viewports.emplace_back(vk::Viewport(0, 0, 1, 1, 0.0f, 1.0f));
        data.viewport.setViewports(vp);
        if (std::ranges::find(data.dynamicStates, vk::DynamicState::eViewport)
                == data.dynamicStates.end())
        {
            data.dynamicStates.emplace_back(vk::DynamicState::eViewport);
        }
    }
    if (data.viewport.scissorCount == 0)
    {
        const auto& sc = data.scissorRects.emplace_back(vk::Rect2D({ 0, 0 }, { 1, 1 }));
        data.viewport.setScissors(sc);
        if (std::ranges::find(data.dynamicStates, vk::DynamicState::eScissor)
                == data.dynamicStates.end())
        {
            data.dynamicStates.emplace_back(vk::DynamicState::eScissor);
        }
    }

    data.vertexInput = vk::PipelineVertexInputStateCreateInfo({}, data.inputBindings, data.attributes);
    data.dynamicState = vk::PipelineDynamicStateCreateInfo({}, data.dynamicStates);
    data.colorBlending.setAttachments(data.colorBlendAttachments);
}



void trc::ComputePipelineTemplate::setProgramCode(std::string code)
{
    shaderCode = std::move(code);
}

auto trc::ComputePipelineTemplate::getShaderCode() const -> const std::string&
{
    return shaderCode;
}

auto trc::ComputePipelineTemplate::getSpecializationConstants() const
    -> const SpecializationConstantStorage&
{
    return specConstants;
}

auto trc::ComputePipelineTemplate::getEntryPoint() const -> const std::string&
{
    return entryPoint;
}



auto trc::makeGraphicsPipeline(
    const vkb::Device& device,
    const PipelineTemplate& _template,
    PipelineLayout& layout,
    vk::RenderPass renderPass,
    ui32 subPass) -> Pipeline
{
    const PipelineDefinitionData& def = _template.getPipelineData();
    const ProgramDefinitionData& shader = _template.getProgramData();

    // Create a program from the shader code
    vkb::ShaderProgram program = shader.makeProgram(device);

    auto pipeline = device->createGraphicsPipelineUnique(
        {},
        vk::GraphicsPipelineCreateInfo(
            {},
            program.getStageCreateInfo(),
            &def.vertexInput,
            &def.inputAssembly,
            &def.tessellation,
            &def.viewport,
            &def.rasterization,
            &def.multisampling,
            &def.depthStencil,
            &def.colorBlending,
            &def.dynamicState,
            *layout,
            renderPass, subPass,
            vk::Pipeline(), 0
        )
    ).value;

    return Pipeline{ layout, std::move(pipeline), vk::PipelineBindPoint::eGraphics };
}

auto trc::makeComputePipeline(
    const vkb::Device& device,
    const ComputePipelineTemplate& _template,
    PipelineLayout& layout) -> Pipeline
{
    auto shaderModule = vkb::makeShaderModule(device, _template.getShaderCode());
    auto pipeline = device->createComputePipelineUnique(
        {},
        vk::ComputePipelineCreateInfo(
            {},
            vk::PipelineShaderStageCreateInfo(
                {}, vk::ShaderStageFlagBits::eCompute,
                *shaderModule,
                _template.getEntryPoint().c_str()
            ),
            *layout
        )
    ).value;

    return Pipeline{ layout, std::move(pipeline), vk::PipelineBindPoint::eCompute };
}
