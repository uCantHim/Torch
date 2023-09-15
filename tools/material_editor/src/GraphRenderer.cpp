#include "GraphRenderer.h"

#include <algorithm>
#include <numeric>
#include <ranges>

#include <trc_util/algorithm/VectorTransform.h>

#include "pipelines.h"


void GraphRenderData::pushNode(vec2 pos, vec2 size, vec4 color)
{
    quads.dimensions.emplace_back(pos, size);
    quads.colors.emplace_back(color);
}

void GraphRenderData::pushSocket(vec2 pos, vec2 size, vec4 color)
{
    quads.dimensions.emplace_back(pos, size);
    quads.colors.emplace_back(color);
}

void GraphRenderData::pushLink(vec2 from, vec2 to, vec4 color)
{
    lines.dimensions.emplace_back(from, to - from);
    lines.colors.emplace_back(color);
}

void GraphRenderData::pushBorder(vec2 pos, vec2 size, vec4 color)
{
    lines.dimensions.emplace_back(pos, vec2{ size.x, 0.0f });
    lines.dimensions.emplace_back(pos, vec2{ 0.0f, size.y });
    lines.dimensions.emplace_back(pos + size, vec2{ -size.x, 0.0f });
    lines.dimensions.emplace_back(pos + size, vec2{ 0.0f, -size.y });
    lines.colors.emplace_back(color);
    lines.colors.emplace_back(color);
    lines.colors.emplace_back(color);
    lines.colors.emplace_back(color);
}

void GraphRenderData::clear()
{
    quads.dimensions.clear();
    quads.colors.clear();
    lines.dimensions.clear();
    lines.colors.clear();
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
        const auto& [nodePos, nodeSize] = layout.nodeSize.get(node);
        res.pushNode(nodePos, nodeSize, graph::kNodeColor);
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
    }
}

void renderHighlightBorders(
    const GraphInteraction& interaction,
    const GraphLayout& layout,
    GraphRenderData& res)
{
    // Create border for the currently hovered element
    if (interaction.hoveredNode)
    {
        const auto [pos, size] = layout.nodeSize.get(*interaction.hoveredNode);
        res.pushBorder(pos, size, graph::kHighlightColor);
    }

    // Create borders for all selected elements
    for (const auto node : interaction.selectedNodes)
    {
        const auto [pos, size] = layout.nodeSize.get(node);
        res.pushBorder(pos, size, graph::kSelectColor);
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
    renderHighlightBorders(scene.interaction, layout, res);
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
    assert(data.quads.colors.size() == data.quads.dimensions.size());
    assert(data.lines.colors.size() == data.lines.dimensions.size());

    auto& dst = *deviceData;
    dst.drawCmds.clear();

    // Modify this if you add a new type of primitive
    auto primitives = { data.quads, data.lines };

    auto counts = primitives | std::views::transform([](auto&& el){ return el.dimensions.size(); });
    const size_t numElems = std::accumulate(counts.begin(), counts.end(), 0);

    const size_t dimBufSize = numElems * kElemDimSize;
    const size_t colorBufSize = numElems * kElemColorSize;
    if (dst.dimensionBuf.size() < dimBufSize)
    {
        assert(dst.colorBuf.size() < colorBufSize);

        dst.dimensionBuf.unmap();
        dst.colorBuf.unmap();

        dst.dimensionBuf = makeVertBuf(device, dimBufSize);
        dst.colorBuf = makeVertBuf(device, colorBufSize);
        dst.dimensions = dst.dimensionBuf.map<vec4*>();
        dst.colors = dst.colorBuf.map<vec4*>();
    }

    // Copy primitive data to device
    size_t offsetElems{ 0 };
    for (const auto& prim : primitives)
    {
        const auto& draw = dst.drawCmds.emplace_back(DeviceData::DrawCmd{
            .indexCount    = prim.geometryType == GraphRenderData::Geometry::eQuad ? 6u : 2u,
            .instanceCount = static_cast<ui32>(prim.dimensions.size()),
            .pipeline      = prim.geometryType == GraphRenderData::Geometry::eQuad
                             ? getGraphElemPipeline()
                             : getLinePipeline(),
        });

        memcpy(dst.dimensions + offsetElems,
               prim.dimensions.data(),
               prim.dimensions.size() * kElemDimSize);
        memcpy(dst.colors + offsetElems,
               prim.colors.data(),
               prim.colors.size() * kElemColorSize);
        offsetElems += draw.instanceCount;
    }
    assert(numElems == offsetElems);

    dst.dimensionBuf.flush();
    dst.colorBuf.flush();
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
