#include "core/PipelineBuilder.h"



trc::GraphicsPipelineBuilder::GraphicsPipelineBuilder(const PipelineTemplate& _template)
    :
    program(_template.getProgramData()),
    data(_template.getPipelineData())
{

}

auto trc::GraphicsPipelineBuilder::setVertexShader(ShaderCode code) -> Self&
{
    program.vertexShaderCode = std::move(code);
    return *this;
}

auto trc::GraphicsPipelineBuilder::setFragmentShader(ShaderCode code) -> Self&
{
    program.fragmentShaderCode = std::move(code);
    return *this;
}

auto trc::GraphicsPipelineBuilder::setGeometryShader(ShaderCode code) -> Self&
{
    program.geometryShaderCode = std::move(code);
    return *this;
}

auto trc::GraphicsPipelineBuilder::setTesselationShader(ShaderCode control, ShaderCode eval)
    -> Self&
{
    program.tesselationControlShaderCode = std::move(control);
    program.tesselationEvaluationShaderCode = std::move(eval);
    return *this;
}

auto trc::GraphicsPipelineBuilder::setProgram(ShaderCode vertex, ShaderCode fragment) -> Self&
{
    program = { std::move(vertex), std::move(fragment), std::nullopt, std::nullopt, std::nullopt };
    return *this;
}

auto trc::GraphicsPipelineBuilder::setProgram(
    ShaderCode vertex,
    ShaderCode geometry,
    ShaderCode fragment) -> Self&
{
    program = {
        std::move(vertex), std::move(fragment),
        std::move(geometry),
        std::nullopt, std::nullopt
    };
    return *this;
}

auto trc::GraphicsPipelineBuilder::setProgram(
    ShaderCode vertex,
    ShaderCode tesc,
    ShaderCode tese,
    ShaderCode fragment) -> Self&
{
    program = {
        std::move(vertex), std::move(fragment),
        std::nullopt,
        std::move(tesc), std::move(tese)
    };
    return *this;
}

auto trc::GraphicsPipelineBuilder::setProgram(
    ShaderCode vertex,
    ShaderCode geometry,
    ShaderCode tesc,
    ShaderCode tese,
    ShaderCode fragment) -> Self&
{
    program = {
        std::move(vertex), std::move(fragment),
        std::move(geometry),
        std::move(tesc), std::move(tese)
    };
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

auto trc::GraphicsPipelineBuilder::build(
    PipelineLayout::ID layout,
    const RenderPassName& renderPass) const -> PipelineTemplate
{
    return { std::move(program), std::move(data), layout, renderPass };
}

auto trc::GraphicsPipelineBuilder::build(
    const vkb::Device& device,
    PipelineLayout& layout,
    vk::RenderPass renderPass,
    ui32 subPass) -> Pipeline
{
    return makeGraphicsPipeline(
        { std::move(program), std::move(data) },
        *device, layout, renderPass, subPass
    );
}



auto trc::buildGraphicsPipeline() -> GraphicsPipelineBuilder
{
    return {};
}
