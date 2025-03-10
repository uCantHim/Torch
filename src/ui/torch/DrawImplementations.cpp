#include "trc/ui/torch/DrawImplementations.h"

#include "trc/DescriptorSetUtils.h"
#include "trc/GuiShaders.h"
#include "trc/base/Buffer.h"
#include "trc/base/Logging.h"
#include "trc/core/PipelineBuilder.h"
#include "trc/ui/FontRegistry.h"
#include "trc/ui/Window.h"



struct QuadVertex
{
    trc::vec2 pos;
    trc::vec2 uv;
};

auto trc::ui_impl::DrawCollector::makeLinePipeline(vk::Format format)
    -> Pipeline
{
    return buildGraphicsPipeline()
        .setProgram(pipelines::getLine())
        .addVertexInputBinding(
            vk::VertexInputBindingDescription(0, sizeof(vec2), vk::VertexInputRate::eVertex),
            { vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32Sfloat, 0) }
        )
        .setPrimitiveTopology(vk::PrimitiveTopology::eLineList)
        .addViewport({})
        .addScissorRect({})
        .addDynamicState(vk::DynamicState::eViewport)
        .addDynamicState(vk::DynamicState::eScissor)
        .addDynamicState(vk::DynamicState::eLineWidth)
        .addColorBlendAttachment(DEFAULT_COLOR_BLEND_ATTACHMENT_DISABLED)
        .setColorBlending({}, false, {}, {})
        .build(device, linePipelineLayout, { 0x00, format, {}, {} });
}

auto trc::ui_impl::DrawCollector::makeQuadPipeline(vk::Format format)
    -> Pipeline
{
    return buildGraphicsPipeline()
        .setProgram(pipelines::getQuad())
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
        .addColorBlendAttachment(vk::PipelineColorBlendAttachmentState{
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
        })
        .setColorBlending({}, false, {}, {})
        .build(device, quadPipelineLayout, { 0x00, format, {}, {} });
}

auto trc::ui_impl::DrawCollector::makeTextPipeline(vk::Format format)
    -> Pipeline
{
    return buildGraphicsPipeline()
        .setProgram(pipelines::getText())
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
        .build(device, quadPipelineLayout, { 0x00, format, {}, {} });
}



void trc::ui_impl::DrawCollector::initStaticResources()
{
    static bool initialized{ false };
    if (initialized) return;
    initialized = true;

    // Set up resource loading callbacks
    ui::initUserCallbacks(
        // Callback on font load
        [] (ui32 fontIndex, const GlyphCache& cache)
        {
            for (auto collector : existingCollectors) {
                collector->addFont(fontIndex, cache);
            }
        },
        // Callback on image load
        [](auto) {}
    );
}



trc::ui_impl::DrawCollector::DrawCollector(const Device& device, vk::Format dstImageFormat)
    :
    device(device),
    quadVertexBuffer(
        device,
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
    lineUvBuffer(
        device,
        std::vector<vec2>{ vec2(0.0f, 0.0f), vec2(1.0f, 1.0f) },
        vk::BufferUsageFlagBits::eVertexBuffer
    ),
    // Create descriptor set layout
    descLayout(trc::buildDescriptorSetLayout()
        .addBinding(vk::DescriptorType::eCombinedImageSampler, 30,
                    vk::ShaderStageFlagBits::eFragment,
                    vk::DescriptorBindingFlagBits::eVariableDescriptorCount
                    | vk::DescriptorBindingFlagBits::eUpdateAfterBind)
        .addFlag(vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool)
        .build(device)
    ),
    linePipelineLayout(makePipelineLayout(device, {},
        {
            // Line start and end
            { vk::ShaderStageFlagBits::eVertex, 0, sizeof(vec2) * 2 },
            // Line color
            { vk::ShaderStageFlagBits::eFragment, sizeof(vec2) * 2, sizeof(vec4) },
        }
    )),
    //quadPipelineLayout(makePipelineLayout(device, {}, {})),
    quadPipelineLayout(trc::makePipelineLayout(device, { *descLayout }, {})),
    textPipelineLayout(trc::makePipelineLayout(device, { *descLayout }, {})),
    linePipeline(makeLinePipeline(dstImageFormat)),
    quadPipeline(makeQuadPipeline(dstImageFormat)),
    textPipeline(makeTextPipeline(dstImageFormat)),
    quadBuffer(
        device,
        100, // Initial max number of quads
        vk::BufferUsageFlagBits::eVertexBuffer,
        vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible
    ),
    letterBuffer(
        device,
        100, // Initial max number of glyphs
        vk::BufferUsageFlagBits::eVertexBuffer,
        vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible
    )
{
    initStaticResources();

    // Create descriptor pool
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

    existingCollectors.push_back(this);
    for (auto [i, cache] : ui::FontRegistry::getFonts()) {
        addFont(i, cache);
    }
}

trc::ui_impl::DrawCollector::~DrawCollector()
{
    auto it = std::find(existingCollectors.begin(), existingCollectors.end(), this);
    if (it != existingCollectors.end()) {
        existingCollectors.erase(it);
    }
}

void trc::ui_impl::DrawCollector::beginFrame()
{
    quadBuffer.clear();
    textRanges.clear();
    letterBuffer.clear();
    lines.clear();
}

void trc::ui_impl::DrawCollector::drawElement(const ui::DrawInfo& info)
{
    std::visit([this, &info](auto type) {
        add(info.pos, info.size, info.style, type);
        if (info.style.borderThickness > 0) {
            add(info.pos, info.size, info.style, _border{});
        }
    }, info.type);
}

void trc::ui_impl::DrawCollector::endFrame(vk::CommandBuffer cmdBuf, uvec2 windowSizePixels)
{
    const uvec2 size = windowSizePixels;
    const vk::Viewport defaultViewport(0, 0, size.x, size.y, 0.0f, 1.0f);
    const vk::Rect2D defaultScissor({ 0, 0 }, { size.x, size.y });

    // Draw all quads
    quadPipeline.bind(cmdBuf);
    cmdBuf.setViewport(0, defaultViewport);
    cmdBuf.setScissor(0, defaultScissor);

    cmdBuf.bindVertexBuffers(0, { *quadVertexBuffer, *quadBuffer }, { 0, 0 });
    cmdBuf.draw(6, quadBuffer.size(), 0, 0);

    // Draw all lines
    if (!lines.empty())
    {
        auto layout = *linePipeline.getLayout();
        linePipeline.bind(cmdBuf);
        cmdBuf.setViewport(0, defaultViewport);
        cmdBuf.setScissor(0, defaultScissor);

        for (const auto& line : lines)
        {
            cmdBuf.pushConstants<vec2>(layout, vk::ShaderStageFlagBits::eVertex,
                                       0, { line.start, line.end });
            cmdBuf.pushConstants<vec4>(layout, vk::ShaderStageFlagBits::eFragment,
                                       16, line.color);
            cmdBuf.setLineWidth(line.width);

            cmdBuf.bindVertexBuffers(0, *lineUvBuffer, vk::DeviceSize(0));
            cmdBuf.draw(2, 1, 0, 0);
        }
    }

    // Draw text
    if (!textRanges.empty())
    {
        textPipeline.bind(cmdBuf);
        cmdBuf.setViewport(0, defaultViewport);

        cmdBuf.bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics, *textPipeline.getLayout(),
            0, *fontDescSet, {}
        );
        cmdBuf.bindVertexBuffers(0, { *quadVertexBuffer, *letterBuffer }, { 0, 0 });
    }

    for (ui32 letterOffset{ 0 };
         auto [offset, extent, numLetters] : textRanges)
    {
        cmdBuf.setScissor(0, vk::Rect2D(
            { static_cast<i32>(offset.x * size.x), static_cast<i32>(offset.y * size.y) },
            { static_cast<ui32>(extent.x * size.x), static_cast<ui32>(extent.y * size.y) }
        ));

        cmdBuf.draw(6, numLetters, 0, letterOffset);

        letterOffset += numLetters;
    }
}

trc::ui_impl::DrawCollector::FontInfo::FontInfo(
    const Device& device,
    ui32 fontIndex,
    const GlyphCache& cache)
    :
    fontIndex(fontIndex),
    glyphCache(cache),
    glyphMap(new GlyphMap(device)),
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
    const ui::types::Line& line)
{
    const vec4 color = std::holds_alternative<vec4>(elem.background)
        ? std::get<vec4>(elem.background)
        : vec4(1.0f);

    lines.push_back({
        .start = pos,
        .end   = pos + size,
        .color = color,
        .width = static_cast<float>(line.width)
    });
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
    const vec4 color = std::holds_alternative<vec4>(elem.foreground)
        ? std::get<vec4>(elem.foreground)
        : vec4(1.0f);

    // Retrieve font
    auto fontIt = fonts.find(text.fontIndex);
    if (fontIt == fonts.end()) return;

    for (const auto& letter : text.letters)
    {
        auto uvs = fontIt->second.getGlyphUvs(letter.characterCode);
        letterBuffer.push({
            .basePos    = pos,
            .pos        = letter.glyphOffset,
            .size       = letter.glyphSize,
            .texCoordLL = uvs.lowerLeft,
            .texCoordUR = uvs.upperRight,
            .bearingY   = letter.bearingY,
            .fontIndex  = text.fontIndex,
            .color = color
        });
    }

    const float width = text.maxDisplayWidth < 0.0f ? 1.0f : text.maxDisplayWidth;
    textRanges.push_back(TextRange{
        .scissorOffset = { text.displayBegin, pos.y },
        .scissorSize   = { width, 1.0f },
        .numLetters = static_cast<ui32>(text.letters.size())
    });
}

void trc::ui_impl::DrawCollector::add(vec2 pos, vec2 size, const ui::ElementStyle& elem, _border)
{
    const vec2 ur{ pos + size };

    // Left line
    lines.push_back({ pos, { pos.x, ur.y }, elem.borderColor, static_cast<float>(elem.borderThickness) });
    // Bottom line
    lines.push_back({ pos, { ur.x, pos.y }, elem.borderColor, static_cast<float>(elem.borderThickness) });
    // Right line
    lines.push_back({ { ur.x, pos.y }, ur, elem.borderColor, static_cast<float>(elem.borderThickness) });
    // Top line
    lines.push_back({ { pos.x, ur.y }, ur, elem.borderColor, static_cast<float>(elem.borderThickness) });
}

void trc::ui_impl::DrawCollector::addFont(ui32 fontIndex, const GlyphCache& glyphCache)
{
    auto [it, success] = fonts.try_emplace(fontIndex, device, fontIndex, glyphCache);
    if (!success) {
        log::warn << "Unable to add font of index " << fontIndex;
        return;
    }

    // Force-load a standard set of glyphs
    for (wchar_t c = 32; c < 256; c++) {
        it->second.getGlyphUvs(c);
    }
    updateFontDescriptor();
}

void trc::ui_impl::DrawCollector::updateFontDescriptor()
{
    if (fonts.size() > 0)
    {
        // Recreate descriptor set with updated descriptor count
        const ui32 descriptorCount = fonts.size();
        vk::StructureChain chain{
            vk::DescriptorSetAllocateInfo(*descPool, *descLayout),
            vk::DescriptorSetVariableDescriptorCountAllocateInfo(1, &descriptorCount)
        };
        fontDescSet.reset();
        fontDescSet = std::move(device->allocateDescriptorSetsUnique(
            chain.get<vk::DescriptorSetAllocateInfo>()
        )[0]);

        // Write desciptors
        std::vector<vk::DescriptorImageInfo> fontInfos;
        for (const auto& [fontIndex, font] : fonts)
        {
            fontInfos.emplace_back(vk::DescriptorImageInfo(
                font.glyphMap->getGlyphImage().getDefaultSampler(), *font.imageView,
                vk::ImageLayout::eShaderReadOnlyOptimal
            ));
        }
        vk::WriteDescriptorSet write(
            *fontDescSet, 0,
            0, fontInfos.size(),
            vk::DescriptorType::eCombinedImageSampler,
            fontInfos.data(),
            nullptr,
            nullptr
        );

        device->updateDescriptorSets(write, {});
    }
}
