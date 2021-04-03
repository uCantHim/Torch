#include "ui/torch/DrawImplementations.h"

#include <vkb/Buffer.h>
#include <vulkan/vulkan.hpp>

#include "PipelineBuilder.h"



struct QuadVertex
{
    trc::vec2 pos;
    trc::vec2 uv;
};

auto trc::ui_impl::DrawCollector::makeQuadPipeline(vk::RenderPass renderPass, ui32 subPass)
    -> Pipeline::ID
{
    vkb::ShaderProgram program(
        TRC_SHADER_DIR"/ui/quad.vert.spv",
        TRC_SHADER_DIR"/ui/quad.frag.spv"
    );

    auto layout = trc::makePipelineLayout({}, {});
    auto pipeline = GraphicsPipelineBuilder::create()
        .setProgram(program)
        .addVertexInputBinding(
            vk::VertexInputBindingDescription(0, sizeof(QuadVertex), vk::VertexInputRate::eVertex),
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

auto trc::ui_impl::DrawCollector::makeTextPipeline(vk::RenderPass renderPass, ui32 subPass)
    -> Pipeline::ID
{
    vkb::ShaderProgram program(
        TRC_SHADER_DIR"/ui/text.vert.spv",
        TRC_SHADER_DIR"/ui/text.frag.spv"
    );

    auto layout = trc::makePipelineLayout(
        { *descLayout },
        {
            vk::PushConstantRange(vk::ShaderStageFlagBits::eVertex, 0, sizeof(vec2))
        }
    );

    auto pipeline = GraphicsPipelineBuilder::create()
        .setProgram(program)
        .addVertexInputBinding(
            vk::VertexInputBindingDescription(0, sizeof(QuadVertex), vk::VertexInputRate::eVertex),
            {
                vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32Sfloat, 0),
                vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32Sfloat, sizeof(vec2)),
            }
        )
        .addVertexInputBinding(
            vk::VertexInputBindingDescription(1, sizeof(LetterData), vk::VertexInputRate::eInstance),
            {
                // Text position:
                vk::VertexInputAttributeDescription(2, 1, vk::Format::eR32G32Sfloat, 0),
                // Glyph offset:
                vk::VertexInputAttributeDescription(3, 1, vk::Format::eR32G32Sfloat, 8),
                // Size:
                vk::VertexInputAttributeDescription(4, 1, vk::Format::eR32G32Sfloat, 16),
                // Glyph texture coords:
                vk::VertexInputAttributeDescription(5, 1, vk::Format::eR32G32Sfloat, 24),
                vk::VertexInputAttributeDescription(6, 1, vk::Format::eR32G32Sfloat, 32),
                // Bearing Y:
                vk::VertexInputAttributeDescription(7, 1, vk::Format::eR32Sfloat, 40),
                vk::VertexInputAttributeDescription(8, 1, vk::Format::eR32Uint, 44),
                // Color:
                vk::VertexInputAttributeDescription(9, 1, vk::Format::eR32G32B32A32Sfloat, 48),
            }
        )
        .addViewport({})
        .addScissorRect({})
        .addDynamicState(vk::DynamicState::eViewport)
        .addDynamicState(vk::DynamicState::eScissor)
        .setCullMode(vk::CullModeFlagBits::eNone)
        .addColorBlendAttachment(
            vk::PipelineColorBlendAttachmentState(
                true,
                vk::BlendFactor::eSrcAlpha,
                vk::BlendFactor::eOneMinusSrcAlpha,
                vk::BlendOp::eAdd,
                vk::BlendFactor::eSrcAlpha,
                vk::BlendFactor::eOneMinusSrcAlpha,
                vk::BlendOp::eAdd,
                vk::ColorComponentFlagBits::eR
                | vk::ColorComponentFlagBits::eG
                | vk::ColorComponentFlagBits::eB
                | vk::ColorComponentFlagBits::eA
            )
        )
        .setColorBlending({}, false, {}, {})
        .build(*vkb::getDevice(), *layout, renderPass, subPass);

    return Pipeline::createAtNextIndex(
        std::move(layout),
        std::move(pipeline),
        vk::PipelineBindPoint::eGraphics
    ).first;
}



void trc::ui_impl::DrawCollector::initStaticResources(
    const vkb::Device& device,
    vk::RenderPass renderPass)
{
    static bool initialized{ false };
    if (initialized) return;
    initialized = true;

    // Create layout
    std::vector<vk::DescriptorSetLayoutBinding> bindings{
        vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eCombinedImageSampler, 30,
                                       vk::ShaderStageFlagBits::eFragment)
    };
    auto bindingFlags = vk::DescriptorBindingFlagBits::eVariableDescriptorCount
                        | vk::DescriptorBindingFlagBits::eUpdateAfterBind;
    vk::StructureChain chain{
        vk::DescriptorSetLayoutCreateInfo(
            vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool,
            bindings
        ),
        vk::DescriptorSetLayoutBindingFlagsCreateInfo(bindingFlags),
    };
    descLayout = device->createDescriptorSetLayoutUnique(chain.get<vk::DescriptorSetLayoutCreateInfo>());

    // Create pipelines
    quadPipeline = makeQuadPipeline(renderPass, 0);
    textPipeline = makeTextPipeline(renderPass, 0);

    // Add de-initialization callback
    vkb::StaticInit{
        []{},
        []() {
            descLayout.reset();
        }
    };
}



trc::ui_impl::DrawCollector::DrawCollector(const vkb::Device& device, vk::RenderPass renderPass)
    :
    device(device),
    quadVertexBuffer(
        std::vector<QuadVertex>{
            { vec2(0, 0), vec2(0, 0) },
            { vec2(1, 1), vec2(1, 1) },
            { vec2(0, 1), vec2(0, 1) },
            { vec2(0, 0), vec2(0, 0) },
            { vec2(1, 0), vec2(1, 0) },
            { vec2(1, 1), vec2(1, 1) }
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
    initStaticResources(device, renderPass);

    createDescriptorSet(device);
}

void trc::ui_impl::DrawCollector::beginFrame(vec2 windowSizePixels)
{
    this->windowSizePixels = windowSizePixels;
    quadBuffer.clear();
    letterBuffer.clear();
}

void trc::ui_impl::DrawCollector::drawElement(const ui::DrawInfo& info)
{
    std::visit([this, &info](auto type) {
        add(info.pos, info.size, info.style, type);
    }, info.type);
}

void trc::ui_impl::DrawCollector::endFrame(vk::CommandBuffer cmdBuf)
{
    const auto size = vkb::getSwapchain().getImageExtent();

    // Draw all quads first
    auto& p = Pipeline::at(quadPipeline);
    cmdBuf.bindPipeline(vk::PipelineBindPoint::eGraphics, *p);
    cmdBuf.setViewport(0, vk::Viewport(0, 0, size.width, size.height, 0.0f, 1.0f));
    cmdBuf.setScissor(0, vk::Rect2D({ 0, 0 }, { size.width, size.height }));

    cmdBuf.bindVertexBuffers(0, { *quadVertexBuffer, *quadBuffer }, { 0, 0 });
    cmdBuf.draw(6, quadBuffer.size(), 0, 0);

    // Draw text
    if (letterBuffer.size() > 0)
    {
        auto& pText = Pipeline::at(textPipeline);
        cmdBuf.bindPipeline(vk::PipelineBindPoint::eGraphics, *pText);
        cmdBuf.setViewport(0, vk::Viewport(0, 0, size.width, size.height, 0.0f, 1.0f));
        cmdBuf.setScissor(0, vk::Rect2D({ 0, 0 }, { size.width, size.height }));

        cmdBuf.bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics, pText.getLayout(),
            0, *descSet, {}
        );
        cmdBuf.pushConstants<vec2>(
            pText.getLayout(), vk::ShaderStageFlagBits::eVertex,
            0, windowSizePixels
        );

        cmdBuf.bindVertexBuffers(0, { *quadVertexBuffer, *letterBuffer }, { 0, 0 });
        cmdBuf.draw(6, letterBuffer.size(), 0, 0);
    }
}

void trc::ui_impl::DrawCollector::addFont(ui32 fontIndex)
{
    auto [it, success] = fonts.try_emplace(fontIndex, device);
    if (!success) {
        return;
    }

    // Force loading a standard set of glyphs
    for (wchar_t c = 32; c < 256; c++) {
        it->second.getGlyphUvs(c);
    }
    updateFontDescriptor();
}

trc::ui_impl::DrawCollector::FontInfo::FontInfo(const vkb::Device&)
    :
    glyphMap(new GlyphMap),
    imageView(glyphMap->getGlyphImage().createView(vk::ImageViewType::e2D, vk::Format::eR8Unorm))
{
}

auto trc::ui_impl::DrawCollector::FontInfo::getGlyphUvs(wchar_t character)
    -> GlyphMap::UvRectangle
{
    auto& [isLoaded, uvs] = glyphTextureCoords[character];
    if (!isLoaded)
    {
        uvs = glyphMap->addGlyph(ui::FontRegistry::getGlyph(fontIndex, character));
        isLoaded = true;
    }

    return uvs;
}

void trc::ui_impl::DrawCollector::add(
    vec2, vec2,
    const ui::ElementStyle&,
    const ui::types::NoType&)
{
}

void trc::ui_impl::DrawCollector::add(
    vec2 pos, vec2 size,
    const ui::ElementStyle& elem,
    const ui::types::Quad&)
{
    const vec4 color = std::holds_alternative<vec4>(elem.background)
        ? std::get<vec4>(elem.background)
        : vec4(1.0f);

    quadBuffer.push({ pos, size, color });
}

void trc::ui_impl::DrawCollector::add(
    vec2 pos, vec2,
    const ui::ElementStyle& elem,
    const ui::types::Text& text)
{
    const vec4 color = std::holds_alternative<vec4>(elem.background)
        ? std::get<vec4>(elem.background)
        : vec4(1.0f);

    // TODO: Add fonts in a callback instead of here, this is trash
    auto fontIt = fonts.find(text.fontIndex);
    if (fontIt == fonts.end())
    {
        addFont(text.fontIndex);
        fontIt = fonts.find(text.fontIndex);
    }

    for (const auto& letter : text.letters)
    {
        auto uvs = fontIt->second.getGlyphUvs(letter.characterCode);
        letterBuffer.push({
            .basePos    = pos,
            .pos        = static_cast<vec2>(letter.glyphOffsetPixels),
            .size       = static_cast<vec2>(letter.glyphSizePixels),
            .texCoordLL = uvs.lowerLeft,
            .texCoordUR = uvs.upperRight,
            .bearingY   = static_cast<float>(letter.bearingYPixels),
            .fontIndex  = text.fontIndex,
            .color = color
        });
    }
}

void trc::ui_impl::DrawCollector::createDescriptorSet(const vkb::Device& device)
{
    std::vector<vk::DescriptorPoolSize> poolSizes{
        vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 30),
    };
    descPool = device->createDescriptorPoolUnique(
        vk::DescriptorPoolCreateInfo(
            vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet
            | vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind,
            1, // max sets
            poolSizes
        )
    );

    // Create set
    const ui32 descriptorCount{ 1 };
    vk::StructureChain chain{
        vk::DescriptorSetAllocateInfo(*descPool, *descLayout),
        vk::DescriptorSetVariableDescriptorCountAllocateInfo(1, &descriptorCount)
    };
    descSet = std::move(device->allocateDescriptorSetsUnique(
        chain.get<vk::DescriptorSetAllocateInfo>()
    )[0]);
}

void trc::ui_impl::DrawCollector::updateFontDescriptor()
{
    if (fonts.size() > 0)
    {
        std::vector<vk::DescriptorImageInfo> fontInfos;
        for (const auto& [fontIndex, font] : fonts)
        {
            fontInfos.emplace_back(vk::DescriptorImageInfo(
                font.glyphMap->getGlyphImage().getDefaultSampler(), *font.imageView,
                vk::ImageLayout::eShaderReadOnlyOptimal
            ));
        }
        vk::WriteDescriptorSet write(
            *descSet, 0,
            0, fontInfos.size(),
            vk::DescriptorType::eCombinedImageSampler,
            fontInfos.data(),
            nullptr,
            nullptr
        );

        device->updateDescriptorSets(write, {});
    }
}
