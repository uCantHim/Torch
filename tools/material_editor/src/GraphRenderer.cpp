#include "GraphRenderer.h"

#include <algorithm>
#include <numeric>
#include <ranges>

#include <trc_util/algorithm/VectorTransform.h>

#include "pipelines.h"


void GraphRenderData::beginGroup()
{
    items.emplace_back();
}

auto GraphRenderData::curGroup() -> DrawGroup&
{
    return items.back();
}

void GraphRenderData::pushNode(vec2 pos, vec2 size, vec4 color)
{
    curGroup().quads.dimensions.emplace_back(pos, size);
    curGroup().quads.colors.emplace_back(color);
}

void GraphRenderData::pushSocket(vec2 pos, vec2 size, vec4 color)
{
    curGroup().quads.dimensions.emplace_back(pos, size);
    curGroup().quads.colors.emplace_back(color);
}

void GraphRenderData::pushLink(vec2 from, vec2 to, vec4 color)
{
    curGroup().lines.dimensions.emplace_back(from, to - from);
    curGroup().lines.colors.emplace_back(color);
}

void GraphRenderData::pushBorder(vec2 pos, vec2 size, vec4 color)
{
    curGroup().lines.dimensions.emplace_back(pos, vec2{ size.x, 0.0f });
    curGroup().lines.dimensions.emplace_back(pos, vec2{ 0.0f, size.y });
    curGroup().lines.dimensions.emplace_back(pos + size, vec2{ -size.x, 0.0f });
    curGroup().lines.dimensions.emplace_back(pos + size, vec2{ 0.0f, -size.y });
    curGroup().lines.colors.emplace_back(color);
    curGroup().lines.colors.emplace_back(color);
    curGroup().lines.colors.emplace_back(color);
    curGroup().lines.colors.emplace_back(color);
}

void GraphRenderData::pushSeparator(vec2 pos, float size, vec4 color)
{
    curGroup().lines.dimensions.emplace_back(pos, vec2(size, 0.0f));
    curGroup().lines.colors.emplace_back(color);
}

void GraphRenderData::clear()
{
    curGroup().quads.dimensions.clear();
    curGroup().quads.colors.clear();
    curGroup().lines.dimensions.clear();
    curGroup().lines.colors.clear();
}

void renderNodes(
    const GraphScene& scene,
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

    for (const auto& [node, _] : graph.nodeInfo.items())
    {
        res.beginGroup();

        const auto& [nodePos, nodeSize] = layout.nodeSize.get(node);
        res.pushNode(nodePos, nodeSize, graph::kNodeColor);
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
    const MaterialGraph& graph,
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

auto buildRenderData(const GraphScene& scene) -> GraphRenderData
{
    const auto& graph = scene.graph;
    const auto& layout = scene.layout;

    GraphRenderData res;
    renderNodes(scene, res);
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

MaterialGraphRenderer::MaterialGraphRenderer(const trc::Device& device, const trc::FrameClock& clock)
    :
    device(device),
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
            .colorBuf=makeVertBuf(device, kElemColorSize * 2000),
            .dimensions=nullptr,
            .colors=nullptr,
        };
        res.dimensions = res.dimensionBuf.map<vec4*>();
        res.colors = res.colorBuf.map<vec4*>();

        return res;
    })
{
    auto pl = getGraphElemPipeline();
    trc::PipelineRegistry::cloneGraphicsPipeline(pl);
}

void MaterialGraphRenderer::uploadData(const GraphRenderData& data)
{
    constexpr auto getIndexCount = [](GraphRenderData::Geometry g) {
        return g == GraphRenderData::Geometry::eQuad ? 6u : 2u;
    };
    constexpr auto getPipeline = [](GraphRenderData::Geometry g) {
        return g == GraphRenderData::Geometry::eQuad
               ? getGraphElemPipeline()
               : getLinePipeline();
    };

    auto& dev = *deviceData;
    dev.drawCmds.clear();

    ensureBufferSize(data, dev, device);

    size_t totalOffset{ 0 };  // Offset into the vertex buffers in number of primitives
    for (const auto& data : data.items)
    {
        assert(data.quads.colors.size() == data.quads.dimensions.size());
        assert(data.lines.colors.size() == data.lines.dimensions.size());

        // Modify this if you add a new type of primitive
        auto primitives = { data.quads, data.lines };

        // Copy primitive data to device
        for (const auto& prim : primitives)
        {
            const auto& draw = dev.drawCmds.emplace_back(DeviceData::DrawCmd{
                .indexCount    = getIndexCount(prim.geometryType),
                .instanceCount = static_cast<ui32>(prim.dimensions.size()),
                .pipeline      = getPipeline(prim.geometryType),
            });

            memcpy(dev.dimensions + totalOffset,
                   prim.dimensions.data(),
                   prim.dimensions.size() * kElemDimSize);
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
    for (const auto& cmd : renderData.items)
    {
        auto primitives = { cmd.lines, cmd.quads };
        auto counts = primitives | std::views::transform([](auto&& el){ return el.dimensions.size(); });
        numElems += std::accumulate(counts.begin(), counts.end(), 0);
    }

    const size_t dimBufSize = numElems * kElemDimSize;
    const size_t colorBufSize = numElems * kElemColorSize;
    if (dev.dimensionBuf.size() < dimBufSize)
    {
        assert(dev.colorBuf.size() < colorBufSize);

        dev.dimensionBuf.unmap();
        dev.colorBuf.unmap();

        dev.dimensionBuf = makeVertBuf(device, dimBufSize);
        dev.colorBuf = makeVertBuf(device, colorBufSize);
        dev.dimensions = dev.dimensionBuf.map<vec4*>();
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

        conf.getPipeline(draw.pipeline).bind(cmdBuf, conf);

        cmdBuf.bindVertexBuffers(2,
            { *data.dimensionBuf,    *data.colorBuf },
            { kElemDimSize * offset, kElemColorSize * offset }
        );
        cmdBuf.drawIndexed(draw.indexCount, draw.instanceCount, 0, 0, 0);
        offset += draw.instanceCount;
    }
}
