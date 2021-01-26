#include "ui/torch/DrawImplementations.h"

#include <vkb/Buffer.h>

#include "Pipeline.h"
#include "PipelineBuilder.h"
using namespace trc;



auto makeQuadPipeline(vk::RenderPass renderPass, ui32 subPass) -> Pipeline::ID
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
                vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32Sfloat, 0)
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

inline std::unique_ptr<vkb::DeviceLocalBuffer> quadVertexBuffer{ nullptr };
inline Pipeline::ID quadPipeline;



void trc::internal::initGuiDraw(vk::RenderPass renderPass)
{
    static bool initialized{ false };
    if (initialized) return;
    initialized = true;

    quadVertexBuffer = std::make_unique<vkb::DeviceLocalBuffer>(
        std::vector<vec2>{
            vec2(0, 0), vec2(1, 1), vec2(0, 1),
            vec2(0, 0), vec2(1, 0), vec2(1, 1)
        },
        vk::BufferUsageFlagBits::eVertexBuffer
    );

    quadPipeline = makeQuadPipeline(renderPass, 0);
}

void trc::internal::cleanupGuiDraw()
{
    quadVertexBuffer.reset();
}



// ------------------------ //
//      Draw functions      //
// ------------------------ //

inline void draw(const ui::ElementDrawInfo& elem, const ui::types::NoType&, vk::CommandBuffer cmdBuf)
{
    // No type, draw a canonical quad
    auto& p = Pipeline::at(quadPipeline);
    cmdBuf.bindPipeline(vk::PipelineBindPoint::eGraphics, *p);
    auto size = vkb::getSwapchain().getImageExtent();
    cmdBuf.setViewport(0, vk::Viewport(0, 0, size.width, size.height, 0.0f, 1.0f));
    cmdBuf.setScissor(0, vk::Rect2D({ 0, 0 }, { size.width, size.height }));

    cmdBuf.pushConstants<vec2>(
        p.getLayout(), vk::ShaderStageFlagBits::eVertex,
        0, { elem.pos, elem.size }
    );
    cmdBuf.pushConstants<vec4>(
        p.getLayout(), vk::ShaderStageFlagBits::eVertex,
        static_cast<ui32>(sizeof(vec2) * 2), std::get<vec4>(elem.background)
    );

    cmdBuf.bindVertexBuffers(0, **quadVertexBuffer, { 0 });
    cmdBuf.draw(6, 1, 0, 0);
}

inline void draw(const ui::ElementDrawInfo& elem, const ui::types::Text& text, vk::CommandBuffer cmdBuf)
{
    std::cout << "UI Text element not implemented!\n";
}

void trc::internal::drawElement(const ui::DrawInfo& info, vk::CommandBuffer cmdBuf)
{
    std::visit([&elem=info.elem, cmdBuf](auto type) {
        draw(elem, type, cmdBuf);
    }, info.type);
}
