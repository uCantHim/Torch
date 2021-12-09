#include "PipelineTemplate.h"

#include "core/Instance.h"
#include "core/PipelineBuilder.h"



auto trc::SpecializationConstantStorage::makeSpecializationInfo() const -> vk::SpecializationInfo
{
    return {
        static_cast<ui32>(entries.size()), entries.data(),
        static_cast<ui32>(data.size()), data.data()
    };
}



trc::PipelineTemplate::PipelineTemplate(ShaderDefinitionData program, PipelineDefinitionData pipeline)
    :
    program(std::move(program)),
    data(std::move(pipeline))
{
    compileData();
}

trc::PipelineTemplate::PipelineTemplate(
    ShaderDefinitionData program,
    PipelineDefinitionData pipeline,
    PipelineLayout::ID layout,
    const RenderPassName& renderPass)
    :
    layout(std::move(layout)),
    renderPassName(std::move(renderPass)),
    program(std::move(program)),
    data(std::move(pipeline))
{
    compileData();
}

auto trc::PipelineTemplate::modify() const -> GraphicsPipelineBuilder
{
    return GraphicsPipelineBuilder{ *this };
}

void trc::PipelineTemplate::setLayout(PipelineLayout::ID _layout)
{
    layout = _layout;
}

void trc::PipelineTemplate::setRenderPass(const RenderPassName& name)
{
    renderPassName = name;
}

auto trc::PipelineTemplate::getLayout() const -> PipelineLayout::ID
{
    return layout;
}

auto trc::PipelineTemplate::getRenderPass() const -> const RenderPassName&
{
    return renderPassName;
}

auto trc::PipelineTemplate::getProgramData() const -> const ShaderDefinitionData&
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

    if (data.viewport.viewportCount == 0
        && std::ranges::find(data.dynamicStates, vk::DynamicState::eViewport)
           != data.dynamicStates.end())
    {
        const auto& vp = data.viewports.emplace_back(vk::Viewport(0, 0, 1, 1, 0.0f, 1.0f));
        data.viewport.setViewports(vp);
    }
    if (data.viewport.scissorCount == 0
        && std::ranges::find(data.dynamicStates, vk::DynamicState::eScissor)
           != data.dynamicStates.end())
    {
        const auto& sc = data.scissorRects.emplace_back(vk::Rect2D({ 0, 0 }, { 1, 1 }));
        data.viewport.setScissors(sc);
    }

    data.vertexInput = vk::PipelineVertexInputStateCreateInfo({}, data.inputBindings, data.attributes);
    data.dynamicState = vk::PipelineDynamicStateCreateInfo({}, data.dynamicStates);
    data.colorBlending.setAttachments(data.colorBlendAttachments);
}



void trc::ComputePipelineTemplate::setProgramCode(std::string code)
{
    shaderCode = std::move(code);
}

auto trc::ComputePipelineTemplate::getLayout() const -> PipelineLayout::ID
{
    return layout;
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
    const PipelineTemplate& _template,
    vk::Device device,
    PipelineLayout& layout,
    vk::RenderPass renderPass,
    ui32 subPass) -> Pipeline
{
    // Create copy because it will be modified
    PipelineDefinitionData def = _template.getPipelineData();
    const ShaderDefinitionData& program = _template.getProgramData();

    // Create a program from the shader code
    const bool hasTess = program.tesselationControlShaderCode.has_value()
                         && program.tesselationEvaluationShaderCode.has_value();
    const bool hasGeom = program.geometryShaderCode.has_value();

    vkb::ShaderProgram prog(
        vkb::createShaderModule(device, program.vertexShaderCode),
        vkb::createShaderModule(device, program.fragmentShaderCode),
        hasGeom ? vkb::createShaderModule(device, program.geometryShaderCode.value())
                : vk::UniqueShaderModule{},
        hasTess ? vkb::createShaderModule(device, program.tesselationControlShaderCode.value())
                : vk::UniqueShaderModule{},
        hasTess ? vkb::createShaderModule(device, program.tesselationEvaluationShaderCode.value())
                : vk::UniqueShaderModule{}
    );

    //prog.setVertexSpecializationConstants(program.vertexSpecConstants.makeSpecializationInfo());

    auto pipeline = device.createGraphicsPipelineUnique(
        {},
        vk::GraphicsPipelineCreateInfo(
            {},
            prog.getStageCreateInfos(),
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
    const ComputePipelineTemplate& _template,
    vk::Device device,
    PipelineLayout& layout) -> Pipeline
{
    auto shaderModule = vkb::createShaderModule(device, _template.getShaderCode());
    auto pipeline = device.createComputePipelineUnique(
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

    return Pipeline{ layout, std::move(pipeline), vk::PipelineBindPoint::eGraphics };
}
