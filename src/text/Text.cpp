#include "text/Text.h"

#include "core/PipelineRegistry.h"
#include "core/PipelineBuilder.h"
#include "core/PipelineLayoutBuilder.h"
#include "AssetRegistry.h"
#include "TorchRenderConfig.h"
#include "TorchResources.h"



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

    auto makeTextPipeline() -> Pipeline::ID;
} // namespace trc



trc::Text::Text(const Instance& instance, Font& font)
    :
    instance(instance),
    font(&font),
    vertexBuffer(instance.getDevice(), makeQuad(), vk::BufferUsageFlagBits::eVertexBuffer)
{
}

void trc::Text::attachToScene(SceneBase& scene)
{
    drawRegistration = scene.registerDrawFunction(
        gBufferRenderStage,
        GBufferPass::SubPasses::transparency,
        getPipeline(),
        [this](const DrawEnvironment& env, vk::CommandBuffer cmdBuf)
        {
            font->getDescriptor().bindDescriptorSet(
                cmdBuf,
                vk::PipelineBindPoint::eGraphics, *env.currentPipeline->getLayout(),
                1
            );
            cmdBuf.pushConstants<mat4>(
                *env.currentPipeline->getLayout(), vk::ShaderStageFlagBits::eVertex,
                0, glm::scale(getGlobalTransform(), vec3(BASE_SCALING))
            );

            cmdBuf.bindVertexBuffers(0, { *vertexBuffer, *glyphBuffer }, { 0, 0 });
            cmdBuf.draw(6, numLetters, 0, 0);
        }
    );
}

void trc::Text::removeFromScene()
{
    drawRegistration = {};
}

void trc::Text::print(std::string_view str)
{
    // Create new buffer if new text exceeds current size
    if (str.size() * sizeof(LetterData) > glyphBuffer.size())
    {
        glyphBuffer = vkb::Buffer(
            instance.getDevice(),
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
            penPosition.y -= font->getLineBreakAdvance();
            penPosition.x = 0.0f;
            continue;
        }

        auto g = font->getGlyph(c);

        /**
         * These calculations are a little bit weird because Torch flips
         * the y-axis with the projection matrix. The glyph data, however,
         * is calculated with the text origin in the upper-left corner.
         */
#ifdef TRC_FLIP_Y_PROJECTION
        buf[i++] = LetterData{
            .texCoordLL=vec2(g.texCoordLL.x, g.texCoordUR.y),
            .texCoordUR=vec2(g.texCoordUR.x, g.texCoordLL.y),
            .glyphOffset=penPosition,
            .glyphSize=g.size,
            .bearingY=g.size.y - g.bearingY
        };
#else
        buf[i++] = LetterData{
            .texCoordLL=g.texCoordLL,
            .texCoordUR=g.texCoordUR,
            .glyphOffset=penPosition,
            .glyphSize=g.size,
            .bearingY=g.bearingY
        };
#endif

        penPosition.x += g.advance;
    }
    glyphBuffer.unmap();

    numLetters = str.size();
}

auto trc::Text::getPipeline() -> Pipeline::ID
{
    static auto id = makeTextPipeline();
    return id;
}



auto trc::makeTextPipeline() -> Pipeline::ID
{
    auto layout = buildPipelineLayout()
        .addDescriptor(DescriptorName{ TorchRenderConfig::GLOBAL_DATA_DESCRIPTOR }, true)
        .addDescriptor(DescriptorName{ TorchRenderConfig::FONT_DESCRIPTOR }, false)
        .addDescriptor(DescriptorName{ TorchRenderConfig::G_BUFFER_DESCRIPTOR }, true)
        .addPushConstantRange({ vk::ShaderStageFlagBits::eVertex, 0, sizeof(mat4) })
        .registerLayout<TorchRenderConfig>();

    // Can't access Text::LetterData struct from here
    constexpr size_t LETTER_DATA_SIZE = sizeof(vec2) * 4 + sizeof(float);

    return buildGraphicsPipeline()
        .setProgram(
            vkb::readFile(TRC_SHADER_DIR"/text/static_text.vert.spv"),
            vkb::readFile(TRC_SHADER_DIR"/text/static_text.frag.spv")
        )
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
        .addViewport(vk::Viewport(0, 0, 1, 1, 0.0f, 1.0f))
        .addScissorRect({ { 0, 0 }, { 1, 1 } })
        .disableBlendAttachments(3)
        .addDynamicState(vk::DynamicState::eViewport)
        .addDynamicState(vk::DynamicState::eScissor)
        .registerPipeline<TorchRenderConfig>(
            layout, RenderPassName{ TorchRenderConfig::TRANSPARENT_G_BUFFER_PASS }
        );
}
