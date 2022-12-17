#include "trc/text/Text.h"

#include "trc/text/UnicodeUtils.h"
#include "trc/TorchRenderConfig.h"
#include "trc/TorchRenderStages.h"
#include "trc/PipelineDefinitions.h"
#include "trc/TextPipelines.h"



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



trc::Text::Text(const Instance& instance, FontHandle font)
    :
    instance(instance),
    font(font),
    vertexBuffer(instance.getDevice(), makeQuad(), vk::BufferUsageFlagBits::eVertexBuffer)
{
}

void trc::Text::attachToScene(SceneBase& scene)
{
    drawRegistration = scene.registerDrawFunction(
        gBufferRenderStage,
        GBufferPass::SubPasses::transparency,
        pipelines::text::getStaticTextPipeline(),
        [this](const DrawEnvironment& env, vk::CommandBuffer cmdBuf)
        {
            font.getDescriptor().bindDescriptorSet(
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

void trc::Text::print(const std::string& str)
{
    // Create new buffer if new text exceeds current size
    if (str.size() * sizeof(LetterData) > glyphBuffer.size())
    {
        glyphBuffer = Buffer(
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
    iterUtf8(str, [&, i = 0](CharCode c) mutable
    {
        if (c == '\n')
        {
            penPosition.y -= font.getLineBreakAdvance();
            penPosition.x = 0.0f;
            return;  // continue
        }

        auto g = font.getGlyph(c);

        buf[i++] = LetterData{
            .texCoordLL=g.texCoordLL,
            .texCoordUR=g.texCoordUR,
            .glyphOffset=penPosition,
            .glyphSize=g.size,
            .bearingY=g.bearingY
        };
        penPosition.x += g.advance;
    });
    glyphBuffer.unmap();

    numLetters = str.size();
}
