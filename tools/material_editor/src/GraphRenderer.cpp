#include "GraphRenderer.h"

#include <ranges>

#include <trc_util/algorithm/VectorTransform.h>

#include "pipelines.h"


void GraphRenderData::pushNode(vec2 pos, vec2 size, vec4 color)
{
    nodeDimensions.emplace_back(pos, size);
    nodeColors.emplace_back(color);
}

void GraphRenderData::pushSocket(vec2 pos, vec2 size, vec4 color)
{
    socketDimensions.emplace_back(pos, size);
    socketColors.emplace_back(color);
}

void GraphRenderData::pushLink(vec2 /*from*/, vec2 /*to*/)
{
}

void GraphRenderData::clear()
{
    nodeDimensions.clear();
    nodeColors.clear();
    socketDimensions.clear();
    socketColors.clear();
}

auto buildRenderData(const MaterialGraph& graph, const GraphLayout& layout) -> GraphRenderData
{
    constexpr vec4 kNodeColor{ 1.0f, 1.0f, 1.0f, 1.0f };

    GraphRenderData res;
    for (const auto& [node, sockets] : graph.inputSockets.items())
    {
        const auto& [pos, size] = layout.nodeSize.get(node);
        res.pushNode(pos, size, kNodeColor);
    }
    for (const auto& [node, sockets] : graph.outputSockets.items())
    {
        const auto& [pos, size] = layout.nodeSize.get(node);
        res.pushNode(pos, size, kNodeColor);
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
                { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 0.0f, 1.0f },
                { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f },

                // UV coordinates
                { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 0.0f, 1.0f },
                { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f },
            };
        }(),
        vk::BufferUsageFlagBits::eVertexBuffer
    ),
    indexBuf(device, std::vector<uint32_t>{ 0, 1, 2, 3, 4, 5 }, vk::BufferUsageFlagBits::eIndexBuffer),
    deviceData(clock, [&](auto){
        DeviceData res{
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
    assert(data.nodeColors.size() == data.nodeDimensions.size());
    assert(data.socketColors.size() == data.socketDimensions.size());


    auto& dst = *deviceData;

    const size_t numElems = data.nodeDimensions.size() + data.socketDimensions.size();
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

    dst.numNodes = data.nodeDimensions.size();
    dst.numSockets = data.socketDimensions.size();
    memcpy(dst.dimensions,                               data.nodeDimensions.data(),   dst.numNodes * kElemDimSize);
    memcpy(dst.dimensions + dst.numNodes * kElemDimSize, data.socketDimensions.data(), dst.numSockets * kElemDimSize);
    memcpy(dst.colors,                                 data.nodeColors.data(),   dst.numNodes * kElemColorSize);
    memcpy(dst.colors + dst.numNodes * kElemColorSize, data.socketColors.data(), dst.numSockets * kElemColorSize);

    dst.dimensionBuf.flush();
    dst.colorBuf.flush();
}

void MaterialGraphRenderer::draw(vk::CommandBuffer cmdBuf, trc::RenderConfig& conf)
{
    const auto& data = *deviceData;

    conf.getPipeline(getGraphElemPipeline()).bind(cmdBuf, conf);

    cmdBuf.bindVertexBuffers(0,
        { *vertexBuf,       *vertexBuf },
        { kVertexPosOffset, kVertexUvOffset }
    );
    cmdBuf.bindIndexBuffer(*indexBuf, 0, vk::IndexType::eUint32);

    // Draw nodes
    cmdBuf.bindVertexBuffers(2,
        { *data.dimensionBuf, *data.colorBuf },
        { 0, 0 }
    );
    cmdBuf.drawIndexed(6, data.numNodes, 0, 0, 0);

    // Draw sockets
    cmdBuf.bindVertexBuffers(2,
        { *data.dimensionBuf, *data.colorBuf },
        { kElemDimSize * data.numNodes, kElemColorSize * data.numNodes }
    );
    cmdBuf.drawIndexed(6, data.numSockets, 0, 0, 0);
}
