#include "Text.h"

#include "PipelineBuilder.h"
#include "Renderer.h"
#include "PipelineDefinitions.h"
#include "TorchResources.h"
#include "AssetRegistry.h"



namespace trc
{
    struct TextVertex
    {
        vec3 pos;
        vec2 uv;
    };

    auto makeQuad() -> std::vector<TextVertex>
    {
        return {
            { { 0, 0, 0 }, { 0, 0 } },
            { { 1, 1, 0 }, { 1, 1 } },
            { { 0, 1, 0 }, { 0, 1 } },
            { { 0, 0, 0 }, { 0, 0 } },
            { { 1, 0, 0 }, { 1, 0 } },
            { { 1, 1, 0 }, { 1, 1 } },
        };
    }
} // namespace trc



vkb::StaticInit trc::Text::_init{
    [] {
        auto quad = makeQuad();
        vertexBuffer = std::make_unique<vkb::DeviceLocalBuffer>(
            quad,
            vk::BufferUsageFlagBits::eVertexBuffer
        );
    }
};

trc::Text::Text(Font& font)
    :
    font(&font)
{

}

void trc::Text::attachToScene(SceneBase& scene)
{
    scene.registerDrawFunction(
        RenderStageTypes::getDeferred(),
        internal::DeferredSubPasses::eTransparencyPass,
        internal::Pipelines::eText,
        [this](const DrawEnvironment& env, vk::CommandBuffer cmdBuf)
        {
            font->getDescriptor().getProvider().bindDescriptorSet(
                cmdBuf,
                vk::PipelineBindPoint::eGraphics, env.currentPipeline->getLayout(),
                1
            );
            cmdBuf.pushConstants<mat4>(
                env.currentPipeline->getLayout(), vk::ShaderStageFlagBits::eVertex,
                0, glm::scale(mat4(1.0f), vec3(BASE_SCALING))
            );

            cmdBuf.bindVertexBuffers(0, { **vertexBuffer, *glyphBuffer }, { 0, 0 });
            cmdBuf.draw(6, numLetters, 0, 0);
        }
    );
    this->scene = &scene;
}

void trc::Text::removeFromScene()
{
    if (scene != nullptr)
    {
        scene->unregisterDrawFunction(drawRegistration);
        scene = nullptr;
    }
}

void trc::Text::print(std::string_view str)
{
    // Create new buffer if new text exceeds current size
    if (str.size() * sizeof(LetterData) > glyphBuffer.size())
    {
        glyphBuffer = vkb::Buffer(
            str.size() * sizeof(LetterData),
            vk::BufferUsageFlagBits::eVertexBuffer,
            vk::MemoryPropertyFlagBits::eDeviceLocal
            | vk::MemoryPropertyFlagBits::eHostVisible
            | vk::MemoryPropertyFlagBits::eHostCoherent
        );
    }

    auto buf = reinterpret_cast<LetterData*>(glyphBuffer.map());
    vec2 penPosition{ 0.0f };
    for (int i = 0; CharCode c : str)
    {
        if (c == '\n')
        {
            penPosition.y += 1.0f;
            penPosition.x = 0.0f;
            continue;
        }

        auto g = font->getGlyph(c);
        buf[i++] = LetterData{
            .texCoordLL=g.texCoordLL,
            .texCoordUR=g.texCoordUR,
            .glyphOffset=penPosition,
            .glyphSize=g.size,
            .bearingY=g.bearingY
        };
        penPosition.x += g.advance;
    }
    glyphBuffer.unmap();

    numLetters = str.size();
}

void trc::makeTextPipeline(const Renderer& renderer)
{
    auto extent = vkb::getSwapchain().getImageExtent();

    auto layout = makePipelineLayout(
        {
            renderer.getGlobalDataDescriptorProvider().getDescriptorSetLayout(),
            FontDescriptor::getLayout(),
            renderer.getDeferredRenderPass().getDescriptorProvider().getDescriptorSetLayout(),
        },
        {
            vk::PushConstantRange(vk::ShaderStageFlagBits::eVertex, 0, sizeof(mat4)),
        }
    );

    vkb::ShaderProgram program(TRC_SHADER_DIR"/text/static_text.vert.spv",
                               TRC_SHADER_DIR"/text/static_text.frag.spv");

    // Can't access Text::LetterData struct from here
    constexpr size_t LETTER_DATA_SIZE = sizeof(vec2) * 4 + sizeof(float);

    auto pipeline = GraphicsPipelineBuilder::create()
        .setProgram(program)
        .addVertexInputBinding(
            vk::VertexInputBindingDescription(0, sizeof(TextVertex), vk::VertexInputRate::eVertex),
            {
                { 0, 0, vk::Format::eR32G32B32Sfloat, 0 },
                { 1, 0, vk::Format::eR32G32Sfloat,    sizeof(vec3) },
            }
        )
        .addVertexInputBinding(
            vk::VertexInputBindingDescription(1, LETTER_DATA_SIZE, vk::VertexInputRate::eInstance),
            {
                { 2, 1, vk::Format::eR32G32Sfloat, 0 },
                { 3, 1, vk::Format::eR32G32Sfloat, sizeof(vec2) * 1 },
                { 4, 1, vk::Format::eR32G32Sfloat, sizeof(vec2) * 2 },
                { 5, 1, vk::Format::eR32G32Sfloat, sizeof(vec2) * 3 },
                { 6, 1, vk::Format::eR32Sfloat,    sizeof(vec2) * 4 },
            }
        )
        .setCullMode(vk::CullModeFlagBits::eNone)
        .addViewport(vk::Viewport(0, 0, extent.width, extent.height, 0.0f, 1.0f))
        .addScissorRect({ { 0, 0 }, extent })
        .addColorBlendAttachment(DEFAULT_COLOR_BLEND_ATTACHMENT_DISABLED)
        .addColorBlendAttachment(DEFAULT_COLOR_BLEND_ATTACHMENT_DISABLED)
        .addColorBlendAttachment(DEFAULT_COLOR_BLEND_ATTACHMENT_DISABLED)
        .addColorBlendAttachment(DEFAULT_COLOR_BLEND_ATTACHMENT_DISABLED)
        .setColorBlending({}, false, vk::LogicOp::eOr, {})
        .build(
            *vkb::getDevice(),
            *layout,
            *renderer.getDeferredRenderPass(),
            internal::DeferredSubPasses::eTransparencyPass
        );

    auto& p = makeGraphicsPipeline(internal::Pipelines::eText, std::move(layout), std::move(pipeline));
    p.addStaticDescriptorSet(0, renderer.getGlobalDataDescriptorProvider());
    p.addStaticDescriptorSet(2, renderer.getDeferredRenderPass().getDescriptorProvider());
}
