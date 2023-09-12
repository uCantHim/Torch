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

auto buildRenderData(const MaterialGraph& graph, const GraphLayout& layout) -> GraphRenderData
{
    GraphRenderData res;
    for (const auto& [node, _] : graph.nodeInfo.items())
    {
        const auto& [pos, size] = layout.nodeSize.get(node);
        res.pushNode(pos, size, graph::kNodeColor);
    }

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
