#include "GraphRenderer.h"

#include <algorithm>
#include <numeric>
#include <ranges>

#include <trc/DescriptorSetUtils.h>
#include <trc/base/Barriers.h>
#include <trc/base/ImageUtils.h>
#include <trc_util/algorithm/VectorTransform.h>

#include "pipelines.h"

// Get an iterable list of all types of primitives in a draw group
#define allPrimitives(group) { (group).quads, (group).glyphs, (group).lines }



void GraphRenderData::Primitive::pushItem(vec4 dim, vec4 uvDim, vec4 color)
{
    dimensions.emplace_back(dim);
    uvDims.emplace_back(uvDim);
    colors.emplace_back(color);
}

void GraphRenderData::Primitive::pushDefaultUvs()
{
    uvDims.emplace_back(kDefaultUv);
}



void GraphRenderData::beginGroup()
{
    groups.emplace_back();
}

auto GraphRenderData::curGroup() -> DrawGroup&
{
    return groups.back();
}

void GraphRenderData::pushNode(vec2 pos, vec2 size, vec4 color)
{
    curGroup().quads.pushItem({ pos, size }, kDefaultUv, color);
}

void GraphRenderData::pushSocket(vec2 pos, vec2 size, vec4 color)
{
    curGroup().quads.pushItem({ pos, size }, kDefaultUv, color);
}

void GraphRenderData::pushLink(vec2 from, vec2 to, vec4 color)
{
    curGroup().lines.pushItem({ from, to - from }, kDefaultUv, color);
}

void GraphRenderData::pushText(vec2 pos, float scale, std::string_view str, vec4 color, Font& font)
{
    for (char c : str)
    {
        const auto& meta = font.getMetadata(c).metaNormalized;
        const auto& g = font.getGlyph(c);

        // Correct position for Vulkan's flipped y-axis
        const vec2 gPos{ pos.x, pos.y + (font.maxAscendNorm - meta.bearingY) * scale };
        const vec2 gSize{ g.size * scale };
        curGroup().glyphs.pushItem({ gPos, gSize }, { g.uvPos, g.uvSize }, color);

        pos.x += meta.advance * scale;
    }
}

void GraphRenderData::pushLetter(vec2 pos, float scale, trc::CharCode c, vec4 color, Font& font)
{
    const auto& g = font.getGlyph(c);
    curGroup().glyphs.dimensions.emplace_back(pos, g.size * scale);
    curGroup().glyphs.uvDims.emplace_back(g.uvPos, g.uvSize);
    curGroup().glyphs.colors.emplace_back(color);
}

void GraphRenderData::pushBorder(vec2 pos, vec2 size, vec4 color)
{
    curGroup().lines.pushItem({ pos, vec2{ size.x, 0.0f } },         kDefaultUv, color);
    curGroup().lines.pushItem({ pos, vec2{ 0.0f, size.y } },         kDefaultUv, color);
    curGroup().lines.pushItem({ pos + size, vec2{ -size.x, 0.0f } }, kDefaultUv, color);
    curGroup().lines.pushItem({ pos + size, vec2{ 0.0f, -size.y } }, kDefaultUv, color);
}

void GraphRenderData::pushSeparator(vec2 pos, float size, vec4 color)
{
    curGroup().lines.pushItem({ pos, vec2(size, 0.0f) }, kDefaultUv, color);
}

void GraphRenderData::clear()
{
    curGroup().quads.dimensions.clear();
    curGroup().quads.uvDims.clear();
    curGroup().quads.colors.clear();
    curGroup().lines.dimensions.clear();
    curGroup().lines.uvDims.clear();
    curGroup().lines.colors.clear();
    curGroup().glyphs.dimensions.clear();
    curGroup().glyphs.uvDims.clear();
    curGroup().glyphs.colors.clear();
}

void renderNodes(
    const GraphScene& scene,
    Font& font,
    GraphRenderData& res)
{
    auto calcSocketColor = [&scene](SocketID sock) -> vec4 {
        if (sock == scene.interaction.hoveredSocket) {
            return graph::applyColorModifier(graph::kSocketColor, graph::kHighlightColorModifier);
        }
        return graph::kSocketColor;
    };

    const auto& graph = scene.graph;
    const auto& layout = scene.layout;

    for (const auto& [node, nodeInfo] : graph.nodeInfo.items())
    {
        res.beginGroup();

        const auto& [nodePos, nodeSize] = layout.nodeSize.get(node);
        const vec2 titlePos = calcTitleTextPos(node, layout);
        res.pushNode(nodePos, nodeSize, graph::kNodeColor);
        res.pushText(titlePos, graph::kTextHeight, nodeInfo.desc.name, graph::kTextColor, font);
        res.pushSeparator(nodePos + vec2(0.0f, graph::kNodeHeaderHeight),
                          nodeSize.x,
                          graph::kSeparatorColor);
        for (const auto& sock : graph.inputSockets.get(node))
        {
            const auto& [sockPos, sockSize] = layout.socketSize.get(sock);
            res.pushSocket(nodePos + sockPos, sockSize, calcSocketColor(sock));
        }
        for (const auto& sock : graph.outputSockets.get(node))
        {
            const auto& [sockPos, sockSize] = layout.socketSize.get(sock);
            res.pushSocket(nodePos + sockPos, sockSize, calcSocketColor(sock));
        }

        if (scene.interaction.selectedNodes.contains(node)) {
            res.pushBorder(nodePos, nodeSize, graph::kSelectColor);
        }
        else if (scene.interaction.hoveredNode == node) {
            res.pushBorder(nodePos, nodeSize, graph::kHighlightColor);
        }
    }
}

void renderSocketLinks(
    const GraphTopology& graph,
    const GraphLayout& layout,
    GraphRenderData& res)
{
    auto calcGlobalSocketPos = [&](SocketID sock) -> vec2 {
        const auto node = graph.socketInfo.get(sock).parentNode;
        const vec2 nodePos = layout.nodeSize.get(node).origin;
        return layout.socketSize.get(sock).origin + nodePos;
    };

    res.beginGroup();
    for (const auto& [from, to] : graph.link.items())
    {
        const auto sizeFrom = layout.socketSize.get(from).extent;
        const auto sizeTo = layout.socketSize.get(to).extent;
        const vec2 a = calcGlobalSocketPos(from) + sizeFrom * 0.5f;
        const vec2 b = calcGlobalSocketPos(to) + sizeTo * 0.5f;
        res.pushLink(a, b, graph::kLinkColor);
    }
}

void renderSelectionBox(const GraphInteraction& interaction, GraphRenderData& res)
{
    res.beginGroup();
    if (auto& box = interaction.multiSelectBox) {
        res.pushBorder(box->origin, box->extent, graph::kSelectColor);
    }
}

auto buildRenderData(const GraphScene& scene, Font& font) -> GraphRenderData
{
    const auto& graph = scene.graph;
    const auto& layout = scene.layout;

    GraphRenderData res;
    renderNodes(scene, font, res);
    renderSocketLinks(graph, layout, res);
    renderSelectionBox(scene.interaction, res);

    return res;
}



/**
 * @brief Create a vertex buffer for material graph primitive data
 */
auto makeVertBuf(const trc::Device& device, size_t size) -> trc::Buffer
{
    return {
        device,
        size,
        vk::BufferUsageFlagBits::eVertexBuffer,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eDeviceLocal
    };
}

MaterialGraphRenderer::MaterialGraphRenderer(
    const trc::Device& device,
    const trc::FrameClock& clock,
    const trc::Image& fontImage)
    :
    device(device),
    descProvider({}, {}),
    baseTexture(trc::makeSinglePixelImage(device, vec4(1.0f))),
    vertexBuf(
        device,
        []{
            return std::vector<vec2>{
                // Vertex locations
                { 0.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f },
                { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f },

                // UV coordinates
                { 0.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f },
                { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f },
            };
        }(),
        vk::BufferUsageFlagBits::eVertexBuffer
    ),
    indexBuf(device, std::vector<uint32_t>{ 0, 1, 2, 3, 4, 5 }, vk::BufferUsageFlagBits::eIndexBuffer),
    deviceData(clock, [&](auto){
        DeviceData res{
            .drawCmds={},
            .dimensionBuf=makeVertBuf(device, kElemDimSize * 2000),
            .uvBuf=makeVertBuf(device, kElemUvSize * 2000),
            .colorBuf=makeVertBuf(device, kElemColorSize * 2000),
            .dimensions=nullptr,
            .uvs=nullptr,
            .colors=nullptr,
        };
        res.dimensions = res.dimensionBuf.map<vec4*>();
        res.uvs = res.uvBuf.map<vec4*>();
        res.colors = res.colorBuf.map<vec4*>();

        return res;
    })
{
    constexpr auto imgLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    constexpr auto descType = vk::DescriptorType::eCombinedImageSampler;
    constexpr size_t setCount = 2;

    // Create texture descriptions
    textures[baseTextureIndex] = Texture{
        .imageView=baseTexture.createView(vk::ImageAspectFlagBits::eColor),
        .sampler=device->createSamplerUnique(
            vk::SamplerCreateInfo{
                vk::SamplerCreateFlags{},
                vk::Filter::eNearest, vk::Filter::eNearest,
                vk::SamplerMipmapMode::eNearest,
                vk::SamplerAddressMode::eRepeat,
                vk::SamplerAddressMode::eRepeat,
                vk::SamplerAddressMode::eRepeat,
                0.0f, false, 0.0f, false, vk::CompareOp::eNever,
                0.0f, 0.0f, vk::BorderColor::eFloatOpaqueWhite, false
            }
        ),
        .descSet{},
    };
    textures[fontTextureIndex] = Texture{
        .imageView=fontImage.createView(vk::ImageAspectFlagBits::eColor),
        .sampler=device->createSamplerUnique(
            vk::SamplerCreateInfo{ {}, vk::Filter::eLinear, vk::Filter::eLinear, }
        ),
        .descSet{},
    };

    // Bring base texture into the correct format
    device.executeCommands(trc::QueueType::graphics, [&](vk::CommandBuffer cmdBuf) {
        trc::imageMemoryBarrier(cmdBuf,
            *baseTexture, vk::ImageLayout::eUndefined, imgLayout, {}, {}, {}, {},
            vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
        );
    });

    // Create descriptors
    auto b = trc::buildDescriptorSetLayout()
        .addBinding(descType, 1, vk::ShaderStageFlagBits::eFragment);
    pool = b.buildPool(device, setCount, vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet);
    layout = b.build(device);
    descProvider = { *layout, {} };

    std::vector<vk::DescriptorSetLayout> numSets(setCount, *layout);
    auto sets = device->allocateDescriptorSetsUnique({ *pool, numSets });
    textures[baseTextureIndex].descSet = std::move(sets[0]);
    textures[fontTextureIndex].descSet = std::move(sets[1]);

    // Update descriptor sets
    auto imgInfo = textures | std::views::transform(
        [](Texture& tex) {
            return vk::DescriptorImageInfo{ *tex.sampler, *tex.imageView, imgLayout };
        }
    );

    std::vector<vk::DescriptorImageInfo> imgInfos(imgInfo.begin(), imgInfo.end());
    device->updateDescriptorSets({
        vk::WriteDescriptorSet{ *textures[0].descSet, 0, 0, descType, imgInfos[0] },
        vk::WriteDescriptorSet{ *textures[1].descSet, 0, 0, descType, imgInfos[1] },
    }, {});
}

void MaterialGraphRenderer::uploadData(const GraphRenderData& data)
{
    constexpr auto getIndexCount = [](GraphRenderData::Geometry g) {
        switch (g) {
            case GraphRenderData::Geometry::eQuad: return 6u;
            case GraphRenderData::Geometry::eLine: return 2u;
            case GraphRenderData::Geometry::eGlyph: return 6u;
        } throw std::logic_error("");
    };
    constexpr auto getTexture = [](GraphRenderData::Geometry g) {
        switch (g) {
            case GraphRenderData::Geometry::eQuad: return baseTextureIndex;
            case GraphRenderData::Geometry::eLine: return baseTextureIndex;
            case GraphRenderData::Geometry::eGlyph: return fontTextureIndex;
        } throw std::logic_error("");
    };
    constexpr auto getPipeline = [](GraphRenderData::Geometry g) {
        switch (g) {
            case GraphRenderData::Geometry::eQuad: return getGraphElemPipeline();
            case GraphRenderData::Geometry::eLine: return getLinePipeline();
            case GraphRenderData::Geometry::eGlyph: return getGraphElemPipeline();
        } throw std::logic_error("");
    };

    auto& dev = *deviceData;
    dev.drawCmds.clear();

    ensureBufferSize(data, dev, device);

    size_t totalOffset{ 0 };  // Offset into the vertex buffers in number of primitives
    for (const auto& group : data.groups)
    {
        assert(group.quads.colors.size() == group.quads.dimensions.size());
        assert(group.lines.colors.size() == group.lines.dimensions.size());
        assert(group.glyphs.colors.size() == group.glyphs.dimensions.size());

        // Copy primitive data to device
        for (const auto& prim : allPrimitives(group))
        {
            const auto& draw = dev.drawCmds.emplace_back(DeviceData::DrawCmd{
                .indexCount    = getIndexCount(prim.geometryType),
                .instanceCount = static_cast<ui32>(prim.dimensions.size()),
                .textureIndex  = getTexture(prim.geometryType),
                .pipeline      = getPipeline(prim.geometryType),
            });

            memcpy(dev.dimensions + totalOffset,
                   prim.dimensions.data(),
                   prim.dimensions.size() * kElemDimSize);
            memcpy(dev.uvs + totalOffset,
                   prim.uvDims.data(),
                   prim.uvDims.size() * kElemUvSize);
            memcpy(dev.colors + totalOffset,
                   prim.colors.data(),
                   prim.colors.size() * kElemColorSize);
            totalOffset += draw.instanceCount;
        }
    }

    dev.dimensionBuf.flush();
    dev.colorBuf.flush();
}

void MaterialGraphRenderer::ensureBufferSize(
    const GraphRenderData& renderData,
    DeviceData& dev,
    const trc::Device& device)
{
    size_t numElems{ 0 };
    for (const auto& group : renderData.groups)
    {
        for (auto& prim : allPrimitives(group)) {
            numElems += prim.dimensions.size();
        }
    }

    const size_t dimBufSize = numElems * kElemDimSize;
    const size_t uvBufSize = numElems * kElemUvSize;
    const size_t colorBufSize = numElems * kElemColorSize;
    if (dev.dimensionBuf.size() < dimBufSize)
    {
        assert(dev.colorBuf.size() < colorBufSize);
        assert(dev.uvBuf.size() < colorBufSize);

        dev.dimensionBuf.unmap();
        dev.uvBuf.unmap();
        dev.colorBuf.unmap();

        dev.dimensionBuf = makeVertBuf(device, dimBufSize);
        dev.uvBuf = makeVertBuf(device, uvBufSize);
        dev.colorBuf = makeVertBuf(device, colorBufSize);
        dev.dimensions = dev.dimensionBuf.map<vec4*>();
        dev.uvs = dev.uvBuf.map<vec4*>();
        dev.colors = dev.colorBuf.map<vec4*>();
    }
}

void MaterialGraphRenderer::draw(vk::CommandBuffer cmdBuf, trc::RenderConfig& conf)
{
    const auto& data = *deviceData;

    cmdBuf.bindVertexBuffers(0,
        { *vertexBuf,       *vertexBuf },
        { kVertexPosOffset, kVertexUvOffset }
    );
    cmdBuf.bindIndexBuffer(*indexBuf, 0, vk::IndexType::eUint32);

    size_t offset{ 0 };
    for (const auto& draw : data.drawCmds)
    {
        if (draw.instanceCount == 0) continue;

        auto& pipeline = conf.getPipeline(draw.pipeline);
        pipeline.bind(cmdBuf, conf);
        cmdBuf.bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics, *pipeline.getLayout(),
            1, *textures[draw.textureIndex].descSet, {}
        );

        cmdBuf.bindVertexBuffers(2,
            { *data.dimensionBuf,    *data.uvBuf,          *data.colorBuf },
            { kElemDimSize * offset, kElemUvSize * offset, kElemColorSize * offset }
        );
        cmdBuf.drawIndexed(draw.indexCount, draw.instanceCount, 0, 0, 0);
        offset += draw.instanceCount;
    }
}

auto MaterialGraphRenderer::getTextureDescriptor() const -> const trc::DescriptorProviderInterface&
{
    return descProvider;
}
