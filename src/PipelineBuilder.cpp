#include "utils/PipelineBuilder.h"



auto trc::GraphicsPipelineBuilder::create() -> GraphicsPipelineBuilder
{
    return {};
}

auto trc::GraphicsPipelineBuilder::setProgram(const vkb::ShaderProgram& program) -> Self
{
    this->program = &program;

    return *this;
}

auto trc::GraphicsPipelineBuilder::addVertexInputBinding(
    vk::VertexInputBindingDescription inputBinding,
    std::vector<vk::VertexInputAttributeDescription> attributes) -> Self
{
    this->inputBindings.push_back(inputBinding);
    for (auto& attr : attributes)
    {
        attr.binding = inputBindings.size() - 1;
        this->attributes.push_back(attr);
    }

    return *this;
}

auto trc::GraphicsPipelineBuilder::setVertexInput(
    const std::vector<vk::VertexInputBindingDescription>& inputBindings,
    const std::vector<vk::VertexInputAttributeDescription>& attributes) -> Self
{
    this->inputBindings = inputBindings;
    this->attributes = attributes;

    return *this;
}

auto trc::GraphicsPipelineBuilder::setInputAssembly(vk::PipelineInputAssemblyStateCreateInfo inputAssembly)
     -> Self
{
    this->inputAssembly = inputAssembly;

    return *this;
}

auto trc::GraphicsPipelineBuilder::setPrimitiveTopology(vk::PrimitiveTopology topo) -> Self
{
    inputAssembly.setTopology(topo);

    return *this;
}

auto trc::GraphicsPipelineBuilder::setTessellation(vk::PipelineTessellationStateCreateInfo tessellation)
    -> Self
{
    this->tessellation = tessellation;

    return *this;
}

auto trc::GraphicsPipelineBuilder::addViewport(vk::Viewport viewport) -> Self
{
    this->viewports.push_back(viewport);

    return *this;
}

auto trc::GraphicsPipelineBuilder::addScissorRect(vk::Rect2D scissorRect) -> Self
{
    this->scissorRects.push_back(scissorRect);

    return *this;
}

auto trc::GraphicsPipelineBuilder::setRasterization(vk::PipelineRasterizationStateCreateInfo rasterization)
    -> Self
{
    this->rasterization = rasterization;

    return *this;
}

auto trc::GraphicsPipelineBuilder::setPolygonMode(vk::PolygonMode polyMode) -> Self
{
    this->rasterization.setPolygonMode(polyMode);

    return *this;
}

auto trc::GraphicsPipelineBuilder::setCullMode(vk::CullModeFlags cullMode) -> Self
{
    this->rasterization.setCullMode(cullMode);

    return *this;
}

auto trc::GraphicsPipelineBuilder::setFrontFace(vk::FrontFace frontFace) -> Self
{
    this->rasterization.setFrontFace(frontFace);

    return *this;
}

auto trc::GraphicsPipelineBuilder::setMultisampling(vk::PipelineMultisampleStateCreateInfo multisampling)
    -> Self
{
    this->multisampling = multisampling;

    return *this;
}

auto trc::GraphicsPipelineBuilder::setSampleCount(vk::SampleCountFlagBits sampleCount) -> Self
{
    this->multisampling.setRasterizationSamples(sampleCount);

    return *this;
}

auto trc::GraphicsPipelineBuilder::setDepthStencilTests(vk::PipelineDepthStencilStateCreateInfo depthStencil)
    -> Self
{
    this->depthStencil = depthStencil;

    return *this;
}

auto trc::GraphicsPipelineBuilder::enableDepthTest() -> Self
{
    this->depthStencil.setDepthTestEnable(true);

    return *this;
}

auto trc::GraphicsPipelineBuilder::disableDepthTest() -> Self
{
    this->depthStencil.setDepthTestEnable(false);

    return *this;
}

auto trc::GraphicsPipelineBuilder::enableDepthWrite() -> Self
{
    this->depthStencil.setDepthWriteEnable(true);

    return *this;
}

auto trc::GraphicsPipelineBuilder::disableDepthWrite() -> Self
{
    this->depthStencil.setDepthWriteEnable(false);

    return *this;
}

auto trc::GraphicsPipelineBuilder::addColorBlendAttachment(
    vk::PipelineColorBlendAttachmentState blendAttachment) -> Self
{
    this->colorBlendAttachments.push_back(blendAttachment);

    return *this;
}

auto trc::GraphicsPipelineBuilder::setColorBlending(
    vk::PipelineColorBlendStateCreateFlags flags,
    vk::Bool32 logicOpEnable,
    vk::LogicOp logicalOperation,
    std::array<float, 4> blendConstants) -> Self
{
    this->colorBlending = vk::PipelineColorBlendStateCreateInfo(
        flags,
        logicOpEnable,
        logicalOperation,
        0, nullptr,
        blendConstants
    );

    return *this;
}

auto trc::GraphicsPipelineBuilder::addDynamicState(vk::DynamicState dynamicState) -> Self
{
    this->dynamicStates.push_back(dynamicState);

    return *this;
}

auto trc::GraphicsPipelineBuilder::build(
    vk::Device device,
    vk::PipelineLayout layout,
    vk::RenderPass renderPass,
    ui32 subPass) -> vk::UniquePipeline
{
    const auto& shaderStages = program->getStageCreateInfos();

    vertexInput = vk::PipelineVertexInputStateCreateInfo(
        vk::PipelineVertexInputStateCreateFlags(),
        static_cast<ui32>(inputBindings.size()), inputBindings.data(),
        static_cast<ui32>(attributes.size()), attributes.data()
    );

    viewport = vk::PipelineViewportStateCreateInfo(
        vk::PipelineViewportStateCreateFlags(),
        static_cast<ui32>(viewports.size()), viewports.data(),
        static_cast<ui32>(scissorRects.size()), scissorRects.data()
    );


    dynamicState = vk::PipelineDynamicStateCreateInfo(
        vk::PipelineDynamicStateCreateFlags(),
        static_cast<ui32>(dynamicStates.size()), dynamicStates.data()
    );

    colorBlending.setAttachmentCount(static_cast<ui32>(colorBlendAttachments.size()));
    colorBlending.setPAttachments(colorBlendAttachments.data());

    return device.createGraphicsPipelineUnique(
        {},
        vk::GraphicsPipelineCreateInfo(
            {},
            static_cast<ui32>(shaderStages.size()), shaderStages.data(),
            &vertexInput,
            &inputAssembly,
            &tessellation,
            &viewport,
            &rasterization,
            &multisampling,
            &depthStencil,
            &colorBlending,
            &dynamicState,
            layout,
            renderPass, subPass,
            vk::Pipeline(), 0
        )
    ).value;
}
