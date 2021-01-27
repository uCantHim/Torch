#include "ui/torch/DrawImplementations.h"

#include <vkb/Buffer.h>

#include "PipelineBuilder.h"



auto trc::ui_impl::DrawCollector::makeQuadPipeline(vk::RenderPass renderPass, ui32 subPass)
    -> Pipeline::ID
{
    vkb::ShaderProgram program(
        TRC_SHADER_DIR"/ui/quad.vert.spv",
        TRC_SHADER_DIR"/ui/quad.frag.spv"
    );

    auto layout = trc::makePipelineLayout(
        {},
        {
            vk::PushConstantRange(
                vk::ShaderStageFlagBits::eVertex,
                0,
                // pos, size
                sizeof(vec2) * 2
                // color
                + sizeof(vec4)
            )
        }
    );

    auto pipeline = GraphicsPipelineBuilder::create()
        .setProgram(program)
        .addVertexInputBinding(
            vk::VertexInputBindingDescription(0, sizeof(vec2), vk::VertexInputRate::eVertex),
            {
                vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32Sfloat, 0),
            }
        )
        .addVertexInputBinding(
            vk::VertexInputBindingDescription(1, sizeof(QuadData), vk::VertexInputRate::eInstance),
            {
                // Position:
                vk::VertexInputAttributeDescription(1, 1, vk::Format::eR32G32Sfloat, 0),
                // Size:
                vk::VertexInputAttributeDescription(2, 1, vk::Format::eR32G32Sfloat, 8),
                // Color:
                vk::VertexInputAttributeDescription(3, 1, vk::Format::eR32G32B32A32Sfloat, 16),
            }
        )
        .addViewport({})
        .addScissorRect({})
        .addDynamicState(vk::DynamicState::eViewport)
        .addDynamicState(vk::DynamicState::eScissor)
        .setCullMode(vk::CullModeFlagBits::eNone)
        .addColorBlendAttachment(DEFAULT_COLOR_BLEND_ATTACHMENT_DISABLED)
        .setColorBlending({}, false, {}, {})
        .build(*vkb::getDevice(), *layout, renderPass, subPass);

    return Pipeline::createAtNextIndex(
        std::move(layout),
        std::move(pipeline),
        vk::PipelineBindPoint::eGraphics
    ).first;
}



inline trc::Pipeline::ID quadPipeline;

void trc::ui_impl::DrawCollector::initStaticResources(vk::RenderPass renderPass)
{
    static bool initialized{ false };
    if (initialized) return;
    initialized = true;

    quadPipeline = makeQuadPipeline(renderPass, 0);
}



trc::ui_impl::DrawCollector::DrawCollector(const vkb::Device& device, vk::RenderPass renderPass)
    :
    quadVertexBuffer(
        std::vector<vec2>{
            vec2(0, 0), vec2(1, 1), vec2(0, 1),
            vec2(0, 0), vec2(1, 0), vec2(1, 1)
        },
        vk::BufferUsageFlagBits::eVertexBuffer
    ),
    quadBuffer(
        vkb::getDevice(),
        100, // Initial max number of quads
        vk::BufferUsageFlagBits::eVertexBuffer,
        vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible
    ),
    letterBuffer(
        vkb::getDevice(),
        100, // Initial max number of glyphs
        vk::BufferUsageFlagBits::eVertexBuffer,
        vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible
    )
{
    initStaticResources(renderPass);

    createDescriptorSet(device);
}

void trc::ui_impl::DrawCollector::beginFrame()
{
    quadBuffer.clear();
    letterBuffer.clear();
}

void trc::ui_impl::DrawCollector::drawElement(const ui::DrawInfo& info)
{
    std::visit([this, &elem=info.elem](auto type) {
        add(elem, type);
    }, info.type);
}

void trc::ui_impl::DrawCollector::endFrame(vk::CommandBuffer cmdBuf)
{
    // Draw all quads first
    auto& p = Pipeline::at(quadPipeline);
    cmdBuf.bindPipeline(vk::PipelineBindPoint::eGraphics, *p);
    auto size = vkb::getSwapchain().getImageExtent();
    cmdBuf.setViewport(0, vk::Viewport(0, 0, size.width, size.height, 0.0f, 1.0f));
    cmdBuf.setScissor(0, vk::Rect2D({ 0, 0 }, { size.width, size.height }));

    cmdBuf.bindVertexBuffers(0, { *quadVertexBuffer, *quadBuffer }, { 0, 0 });
    cmdBuf.draw(6, quadBuffer.size(), 0, 0);
}

void trc::ui_impl::DrawCollector::add(
    const ui::ElementDrawInfo& elem,
    const ui::types::NoType&)
{
    const vec4 color = std::holds_alternative<vec4>(elem.background)
        ? std::get<vec4>(elem.background)
        : vec4(1.0f);

    quadBuffer.push({ elem.pos, elem.size, color });
}

void trc::ui_impl::DrawCollector::add(
    const ui::ElementDrawInfo& elem,
    const ui::types::Text& text)
{
    for (const auto& letter : text.letters)
    {
        letterBuffer.push({
            .pos        = elem.pos,
            .size       = elem.size,
            .texCoordLL = letter.texCoordLL,
            .texCoordUR = letter.texCoordUR,
            .bearingY   = letter.bearingY,
            .fontIndex  = text.fontIndex
        });
    }
}

void trc::ui_impl::DrawCollector::createDescriptorSet(const vkb::Device& device)
{
    //descPool = device->createDescriptorPoolUnique(
    //    vk::DescriptorPoolCreateInfo(
    //        vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
    //        1, // max sets
    //        std::vector<vk::DescriptorPoolSize>{
    //            //vk::DescriptorPoolSize(),
    //        }
    //    )
    //);
}
