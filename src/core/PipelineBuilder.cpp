#include "core/PipelineBuilder.h"



trc::GraphicsPipelineBuilder::GraphicsPipelineBuilder(const PipelineTemplate& _template)
    :
    program(_template.getProgramData()),
    data(_template.getPipelineData())
{
}

auto trc::GraphicsPipelineBuilder::setVertexShader(ShaderCode code) -> Self&
{
    return setShader(vk::ShaderStageFlagBits::eVertex, std::move(code));
}

auto trc::GraphicsPipelineBuilder::setFragmentShader(ShaderCode code) -> Self&
{
    return setShader(vk::ShaderStageFlagBits::eFragment, std::move(code));
}

auto trc::GraphicsPipelineBuilder::setGeometryShader(ShaderCode code) -> Self&
{
    return setShader(vk::ShaderStageFlagBits::eGeometry, std::move(code));
}

auto trc::GraphicsPipelineBuilder::setTesselationShader(ShaderCode control, ShaderCode eval)
    -> Self&
{
    setShader(vk::ShaderStageFlagBits::eTessellationControl, std::move(control));
    setShader(vk::ShaderStageFlagBits::eTessellationEvaluation, std::move(eval));
    return *this;
}

auto trc::GraphicsPipelineBuilder::setShader(vk::ShaderStageFlagBits stage, ShaderCode code)
    -> Self&
{
    auto [it, _] = program.stages.try_emplace(stage);
    it->second.code = std::move(code);
    return *this;
}

auto trc::GraphicsPipelineBuilder::setProgram(ProgramDefinitionData _program) -> Self&
{
    program = std::move(_program);
    return *this;
}

auto trc::GraphicsPipelineBuilder::setProgram(ShaderCode vertex, ShaderCode fragment) -> Self&
{
    program.stages = {
        { vk::ShaderStageFlagBits::eVertex, { std::move(vertex) } },
        { vk::ShaderStageFlagBits::eFragment, { std::move(fragment) } },
    };
    return *this;
}

auto trc::GraphicsPipelineBuilder::setProgram(
    ShaderCode vertex,
    ShaderCode geometry,
    ShaderCode fragment) -> Self&
{
    program.stages = {
        { vk::ShaderStageFlagBits::eVertex, { std::move(vertex) } },
        { vk::ShaderStageFlagBits::eGeometry, { std::move(geometry) } },
        { vk::ShaderStageFlagBits::eFragment, { std::move(fragment) } },
    };
    return *this;
}

auto trc::GraphicsPipelineBuilder::setProgram(
    ShaderCode vertex,
    ShaderCode tesc,
    ShaderCode tese,
    ShaderCode fragment) -> Self&
{
    program.stages = {
        { vk::ShaderStageFlagBits::eVertex, { std::move(vertex) } },
        { vk::ShaderStageFlagBits::eTessellationControl, { std::move(tesc) } },
        { vk::ShaderStageFlagBits::eTessellationEvaluation, { std::move(tese) } },
        { vk::ShaderStageFlagBits::eFragment, { std::move(fragment) } },
    };
    return *this;
}

auto trc::GraphicsPipelineBuilder::setProgram(
    ShaderCode vertex,
    ShaderCode tesc,
    ShaderCode tese,
    ShaderCode geometry,
    ShaderCode fragment) -> Self&
{
    program.stages = {
        { vk::ShaderStageFlagBits::eVertex, { std::move(vertex) } },
        { vk::ShaderStageFlagBits::eTessellationControl, { std::move(tesc) } },
        { vk::ShaderStageFlagBits::eTessellationEvaluation, { std::move(tese) } },
        { vk::ShaderStageFlagBits::eGeometry, { std::move(geometry) } },
        { vk::ShaderStageFlagBits::eFragment, { std::move(fragment) } },
    };
    return *this;
}

auto trc::GraphicsPipelineBuilder::setMeshShadingProgram(
    std::optional<ShaderCode> task,
    ShaderCode mesh,
    ShaderCode fragment) -> Self&
{
    program.stages = {
        { vk::ShaderStageFlagBits::eMeshNV, { std::move(mesh) } },
        { vk::ShaderStageFlagBits::eFragment, { std::move(fragment) } },
    };
    if (task.has_value())
    {
        program.stages[vk::ShaderStageFlagBits::eTaskNV].code = task.value();
    }

    return *this;
}

auto trc::GraphicsPipelineBuilder::addVertexInputBinding(
    vk::VertexInputBindingDescription inputBinding,
    std::vector<vk::VertexInputAttributeDescription> attributes) -> Self&
{
    data.inputBindings.push_back(inputBinding);
    for (auto& attr : attributes)
    {
        attr.binding = data.inputBindings.size() - 1;
        data.attributes.push_back(attr);
    }

    return *this;
}

auto trc::GraphicsPipelineBuilder::setVertexInput(
    const std::vector<vk::VertexInputBindingDescription>& inputBindings,
    const std::vector<vk::VertexInputAttributeDescription>& attributes) -> Self&
{
    data.inputBindings = inputBindings;
    data.attributes = attributes;

    return *this;
}

auto trc::GraphicsPipelineBuilder::setInputAssembly(vk::PipelineInputAssemblyStateCreateInfo inputAssembly)
     -> Self&
{
    data.inputAssembly = inputAssembly;

    return *this;
}

auto trc::GraphicsPipelineBuilder::setPrimitiveTopology(vk::PrimitiveTopology topo) -> Self&
{
    data.inputAssembly.setTopology(topo);

    return *this;
}

auto trc::GraphicsPipelineBuilder::setTessellation(vk::PipelineTessellationStateCreateInfo tessellation)
    -> Self&
{
    data.tessellation = tessellation;

    return *this;
}

auto trc::GraphicsPipelineBuilder::addViewport(vk::Viewport viewport) -> Self&
{
    data.viewports.push_back(viewport);

    return *this;
}

auto trc::GraphicsPipelineBuilder::addScissorRect(vk::Rect2D scissorRect) -> Self&
{
    data.scissorRects.push_back(scissorRect);

    return *this;
}

auto trc::GraphicsPipelineBuilder::setRasterization(vk::PipelineRasterizationStateCreateInfo rasterization)
    -> Self&
{
    data.rasterization = rasterization;

    return *this;
}

auto trc::GraphicsPipelineBuilder::setPolygonMode(vk::PolygonMode polyMode) -> Self&
{
    data.rasterization.setPolygonMode(polyMode);

    return *this;
}

auto trc::GraphicsPipelineBuilder::setCullMode(vk::CullModeFlags cullMode) -> Self&
{
    data.rasterization.setCullMode(cullMode);

    return *this;
}

auto trc::GraphicsPipelineBuilder::setFrontFace(vk::FrontFace frontFace) -> Self&
{
    data.rasterization.setFrontFace(frontFace);

    return *this;
}

auto trc::GraphicsPipelineBuilder::setMultisampling(vk::PipelineMultisampleStateCreateInfo multisampling)
    -> Self&
{
    data.multisampling = multisampling;

    return *this;
}

auto trc::GraphicsPipelineBuilder::setSampleCount(vk::SampleCountFlagBits sampleCount) -> Self&
{
    data.multisampling.setRasterizationSamples(sampleCount);

    return *this;
}

auto trc::GraphicsPipelineBuilder::setDepthStencilTests(vk::PipelineDepthStencilStateCreateInfo depthStencil)
    -> Self&
{
    data.depthStencil = depthStencil;

    return *this;
}

auto trc::GraphicsPipelineBuilder::enableDepthTest() -> Self&
{
    data.depthStencil.setDepthTestEnable(true);

    return *this;
}

auto trc::GraphicsPipelineBuilder::disableDepthTest() -> Self&
{
    data.depthStencil.setDepthTestEnable(false);

    return *this;
}

auto trc::GraphicsPipelineBuilder::enableDepthWrite() -> Self&
{
    data.depthStencil.setDepthWriteEnable(true);

    return *this;
}

auto trc::GraphicsPipelineBuilder::disableDepthWrite() -> Self&
{
    data.depthStencil.setDepthWriteEnable(false);

    return *this;
}

auto trc::GraphicsPipelineBuilder::addColorBlendAttachment(
    vk::PipelineColorBlendAttachmentState blendAttachment) -> Self&
{
    data.colorBlendAttachments.push_back(blendAttachment);

    return *this;
}

auto trc::GraphicsPipelineBuilder::setColorBlending(
    vk::PipelineColorBlendStateCreateFlags flags,
    vk::Bool32 logicOpEnable,
    vk::LogicOp logicalOperation,
    std::array<float, 4> blendConstants) -> Self&
{
    data.colorBlending = vk::PipelineColorBlendStateCreateInfo(
        flags,
        logicOpEnable,
        logicalOperation,
        0, nullptr,
        blendConstants
    );

    return *this;
}

auto trc::GraphicsPipelineBuilder::disableBlendAttachments(ui32 numAttachments) -> Self&
{
    for (ui32 i = 0; i < numAttachments; i++) {
        addColorBlendAttachment(DEFAULT_COLOR_BLEND_ATTACHMENT_DISABLED);
    }
    setColorBlending({}, false, {}, {});

    return *this;
}

auto trc::GraphicsPipelineBuilder::addDynamicState(vk::DynamicState dynamicState) -> Self&
{
    data.dynamicStates.push_back(dynamicState);

    return *this;
}

auto trc::GraphicsPipelineBuilder::build() const -> PipelineTemplate
{
    return { program, data };
}

auto trc::GraphicsPipelineBuilder::build(
    const vkb::Device& device,
    PipelineLayout& layout,
    vk::RenderPass renderPass,
    ui32 subPass) -> Pipeline
{
    return makeGraphicsPipeline(device, build(), layout, renderPass, subPass);
}



auto trc::buildGraphicsPipeline() -> GraphicsPipelineBuilder
{
    return {};
}

auto trc::buildGraphicsPipeline(const PipelineTemplate& t) -> GraphicsPipelineBuilder
{
    return GraphicsPipelineBuilder(t);
}
